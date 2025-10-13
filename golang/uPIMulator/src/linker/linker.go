package linker

import (
	"fmt"
	"os"
	"path/filepath"
	"uPIMulator/src/core"
	"uPIMulator/src/linker/kernel"
	"uPIMulator/src/linker/lexer"
	"uPIMulator/src/linker/logic"
	"uPIMulator/src/linker/parser"
	"uPIMulator/src/misc"
)

type Linker struct {
	command_line_parser *misc.CommandLineParser

	root_dirpath           string
	bin_dirpath            string
	benchmark              string
	num_simulation_threads int

	benchmark_relocatable *kernel.Relocatable
	sdk_relocatables      map[string]*kernel.Relocatable

	executable *kernel.Executable

	linker_script *logic.LinkerScript
}

func (this *Linker) Init(command_line_parser *misc.CommandLineParser) {
	this.command_line_parser = command_line_parser

	this.root_dirpath = command_line_parser.StringParameter("root_dirpath")
	this.bin_dirpath = command_line_parser.StringParameter("bin_dirpath")
	this.benchmark = command_line_parser.StringParameter("benchmark")
	this.num_simulation_threads = int(command_line_parser.IntParameter("num_simulation_threads"))

	this.InitBenchmarkRelocatable()
	this.InitSdkRelocatables()

	this.executable = new(kernel.Executable)
	this.executable.Init(this.benchmark)

	this.linker_script = new(logic.LinkerScript)
	this.linker_script.Init(command_line_parser)
}

func (this *Linker) InitBenchmarkRelocatable() {
	benchmark_build_dirpath := filepath.Join(this.root_dirpath, "benchmark", "build")

	assembly_path := filepath.Join(
		benchmark_build_dirpath,
		this.benchmark,
		"dpu",
		"CMakeFiles",
		fmt.Sprintf("%s_device.dir", this.benchmark),
		"task.c.o",
	)

	this.benchmark_relocatable = new(kernel.Relocatable)
	this.benchmark_relocatable.Init(this.benchmark)
	this.benchmark_relocatable.SetPath(assembly_path)
}

func (this *Linker) InitSdkRelocatables() {
	this.sdk_relocatables = make(map[string]*kernel.Relocatable, 0)

	sdk_build_dirpath := filepath.Join(this.root_dirpath, "sdk", "build")

	sdk_build_dir_entries, sdk_build_dir_read_err := os.ReadDir(sdk_build_dirpath)

	if sdk_build_dir_read_err != nil {
		panic(sdk_build_dir_read_err)
	}

	for _, sdk_build_dir_entry := range sdk_build_dir_entries {
		if sdk_build_dir_entry.IsDir() && sdk_build_dir_entry.Name() != "CMakeFiles" {
			sdk_lib_dirpath := filepath.Join(
				sdk_build_dirpath,
				sdk_build_dir_entry.Name(),
				"CMakeFiles",
				sdk_build_dir_entry.Name()+".dir",
			)

			sdk_lib_dir_entries, sdk_lib_dir_read_err := os.ReadDir(sdk_lib_dirpath)

			if sdk_lib_dir_read_err != nil {
				panic(sdk_lib_dir_read_err)
			}

			for _, sdk_lib_dir_entry := range sdk_lib_dir_entries {
				assembly_path := filepath.Join(sdk_lib_dirpath, sdk_lib_dir_entry.Name())

				lib_dir_name := filepath.Base(sdk_lib_dirpath)
				sdk_relocatable_name := lib_dir_name[:len(lib_dir_name)-4] + "." + sdk_lib_dir_entry.Name()[:len(sdk_lib_dir_entry.Name())-4]

				sdk_relocatable := new(kernel.Relocatable)
				sdk_relocatable.Init(sdk_relocatable_name)
				sdk_relocatable.SetPath(assembly_path)

				this.sdk_relocatables[sdk_relocatable_name] = sdk_relocatable
			}
		}
	}
}

func (this *Linker) Link() {
	this.Lex()
	this.Parse()
	this.AnalyzeLiveness()
	this.MakeExecutable()
	this.LoadExecutable()
	this.DumpExecutable()
}

func (this *Linker) Lex() {
	thread_pool := new(core.ThreadPool)
	thread_pool.Init(this.num_simulation_threads)

	benchmark_lex_job := new(LexJob)
	benchmark_lex_job.Init(this.benchmark_relocatable)

	thread_pool.Enque(benchmark_lex_job)

	for _, sdk_relocatable := range this.sdk_relocatables {
		sdk_lex_job := new(LexJob)
		sdk_lex_job.Init(sdk_relocatable)

		thread_pool.Enque(sdk_lex_job)
	}

	thread_pool.Start()
}

func (this *Linker) Parse() {
	thread_pool := new(core.ThreadPool)
	thread_pool.Init(this.num_simulation_threads)

	benchmark_parse_job := new(ParseJob)
	benchmark_parse_job.Init(this.benchmark_relocatable)

	thread_pool.Enque(benchmark_parse_job)

	for _, sdk_relocatable := range this.sdk_relocatables {
		sdk_parse_job := new(ParseJob)
		sdk_parse_job.Init(sdk_relocatable)

		thread_pool.Enque(sdk_parse_job)
	}

	thread_pool.Start()
}

func (this *Linker) AnalyzeLiveness() {
	thread_pool := new(core.ThreadPool)
	thread_pool.Init(this.num_simulation_threads)

	benchmark_analyze_liveness_job := new(AnalyzeLivenessJob)
	benchmark_analyze_liveness_job.Init(this.benchmark_relocatable)

	thread_pool.Enque(benchmark_analyze_liveness_job)

	for _, sdk_relocatable := range this.sdk_relocatables {
		sdk_analyze_liveness_job := new(AnalyzeLivenessJob)
		sdk_analyze_liveness_job.Init(sdk_relocatable)

		thread_pool.Enque(sdk_analyze_liveness_job)
	}

	thread_pool.Start()
}

func (this *Linker) MakeExecutable() {
	fmt.Printf("Resolving symbols of %s...\n", this.executable.Name())

	this.executable.SetBenchmarkRelocatable(this.benchmark_relocatable)
	this.ResolveSymbols()

	executable_path := filepath.Join(this.bin_dirpath, "main.S")

	fmt.Printf("Dumping the executable to %s...\n", this.executable.Path())

	this.executable.SetPath(executable_path)
	this.executable.DumpAssembly()
}

func (this *Linker) HasResolved() bool {
	for unresolved_symbol, _ := range this.executable.Liveness().UnresolvedSymbols() {
		if !this.linker_script.HasLinkerConstant(unresolved_symbol) {
			return false
		}
	}
	return true
}

func (this *Linker) ResolveSymbols() {
	this.executable.AddSdkRelocatable(this.sdk_relocatables["misc.crt0"])

	iter := 0
	for !this.HasResolved() {
		iter++
		madeProgress := false

		unresolved := this.executable.Liveness().UnresolvedSymbols()
		fmt.Printf("ResolveSymbols: iteration=%d unresolved_count=%d\n", iter, len(unresolved))

		for unresolved_symbol := range unresolved {
			fmt.Println("Resolving symbol:", unresolved_symbol)
			if this.linker_script.HasLinkerConstant(unresolved_symbol) {
				continue
			}

			suffix := "." + unresolved_symbol

			for sdk_name, sdk_relocatable := range this.sdk_relocatables {
				// exact global match
				if _, found := sdk_relocatable.Liveness().GlobalSymbols()[unresolved_symbol]; found {
					fmt.Printf("  -> adding relocatable %s to satisfy %s (global)\n", sdk_name, unresolved_symbol)
					this.executable.AddSdkRelocatable(sdk_relocatable)
					madeProgress = true
					continue
				}

				// exact def match
				if _, found := sdk_relocatable.Liveness().Defs()[unresolved_symbol]; found {
					fmt.Printf("  -> adding relocatable %s to satisfy %s (def)\n", sdk_name, unresolved_symbol)
					this.executable.AddSdkRelocatable(sdk_relocatable)
					madeProgress = true
					continue
				}

				// suffix match for renamed/local symbols (e.g., misc.crt0.STACK_SIZE_TASKLET_0)
				added := false
				for defName := range sdk_relocatable.Liveness().Defs() {
					if len(defName) > len(suffix) && defName[len(defName)-len(suffix):] == suffix {
						fmt.Printf("  -> adding relocatable %s to satisfy %s (def-suffix %s)\n", sdk_name, unresolved_symbol, defName)
						this.executable.AddSdkRelocatable(sdk_relocatable)
						madeProgress = true
						added = true
						break
					}
				}
				if added {
					continue
				}
				for gName := range sdk_relocatable.Liveness().GlobalSymbols() {
					if len(gName) > len(suffix) && gName[len(gName)-len(suffix):] == suffix {
						fmt.Printf("  -> adding relocatable %s to satisfy %s (global-suffix %s)\n", sdk_name, unresolved_symbol, gName)
						this.executable.AddSdkRelocatable(sdk_relocatable)
						madeProgress = true
						break
					}
				}
			}
		}

		if !madeProgress {
			fmt.Println("ResolveSymbols: no progress made while resolving symbols. Dumping diagnostics:")
			fmt.Println("Unresolved symbols:")
			for s := range this.executable.Liveness().UnresolvedSymbols() {
				fmt.Println("  ", s)
			}

			fmt.Println("SDK relocatables and their global symbol counts:")
			for sdk_name, sdk_relocatable := range this.sdk_relocatables {
				fmt.Printf("  %s: %d globals, %d defs\n", sdk_name, len(sdk_relocatable.Liveness().GlobalSymbols()), len(sdk_relocatable.Liveness().Defs()))
			}

			// detailed lookup for unresolved non-constants
			fmt.Println("Detailed per-symbol lookup (non-constant unresolved symbols):")
			for s := range this.executable.Liveness().UnresolvedSymbols() {
				if this.linker_script.HasLinkerConstant(s) {
					continue
				}
				foundAny := false
				fmt.Printf("  %s ->\n", s)
				for sdk_name, sdk_relocatable := range this.sdk_relocatables {
					if _, found := sdk_relocatable.Liveness().GlobalSymbols()[s]; found {
						fmt.Printf("    found in %s (global)\n", sdk_name)
						foundAny = true
					}
					if _, found := sdk_relocatable.Liveness().Defs()[s]; found {
						fmt.Printf("    found in %s (def)\n", sdk_name)
						foundAny = true
					}
					// suffix checks
					suffix := "." + s
					for defName := range sdk_relocatable.Liveness().Defs() {
						if len(defName) > len(suffix) && defName[len(defName)-len(suffix):] == suffix {
							fmt.Printf("    found-suffix in %s: %s\n", sdk_name, defName)
							foundAny = true
						}
					}
					for gName := range sdk_relocatable.Liveness().GlobalSymbols() {
						if len(gName) > len(suffix) && gName[len(gName)-len(suffix):] == suffix {
							fmt.Printf("    found-global-suffix in %s: %s\n", sdk_name, gName)
							foundAny = true
						}
					}
				}
				if !foundAny {
					fmt.Printf("    none\n")
				}
			}

			panic("ResolveSymbols: cannot make progress resolving symbols")
		}
	}
}

func (this *Linker) LoadExecutable() {
	fmt.Println("Re-lexing executable")
	lexer_ := new(lexer.Lexer)
	lexer_.Init()
	token_stream := lexer_.Lex(this.executable.Path())
	this.executable.SetTokenStream(token_stream)

	fmt.Println("Re-parsing executable...")
	parser_ := new(parser.Parser)
	parser_.Init()
	ast := parser_.Parse(token_stream)
	this.executable.SetAst(ast)

	fmt.Println("Assigning labels...")
	label_assigner := new(logic.LabelAssigner)
	label_assigner.Init()
	label_assigner.Assign(this.executable)

	fmt.Println("Assigning addresses..")
	this.linker_script.Assign(this.executable)

	fmt.Println("Setting alias labels...")
	set_assigner := new(logic.SetAssigner)
	set_assigner.Init()
	set_assigner.Assign(this.executable)

	fmt.Println("Assigning instructions...")
	instruction_assigner := new(logic.InstructionAssigner)
	instruction_assigner.Init(this.linker_script)
	instruction_assigner.Assign(this.executable)
}

func (this *Linker) DumpExecutable() {
	this.linker_script.DumpValues(filepath.Join(this.bin_dirpath, "values.txt"))
	this.executable.DumpAddresses(filepath.Join(this.bin_dirpath, "addresses.txt"))
	this.executable.DumpAtomic(filepath.Join(this.bin_dirpath, "atomic.bin"))
	this.executable.DumpIram(filepath.Join(this.bin_dirpath, "iram.bin"))
	this.executable.DumpWram(filepath.Join(this.bin_dirpath, "wram.bin"))
	this.executable.DumpMram(filepath.Join(this.bin_dirpath, "mram.bin"))
}
