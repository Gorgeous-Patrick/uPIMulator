package compiler

import (
	"os"
	"os/exec"
	"strconv"
	"uPIMulator/src/misc"
)

type Compiler struct {
	command_line_parser *misc.CommandLineParser

	root_dirpath string
	benchmark    string

	num_dpus     int
	num_tasklets int
}

func (this *Compiler) Init(command_line_parser *misc.CommandLineParser) {
	this.command_line_parser = command_line_parser

	this.root_dirpath = command_line_parser.StringParameter("root_dirpath")
	this.benchmark = command_line_parser.StringParameter("benchmark")

	num_channels := int(command_line_parser.IntParameter("num_channels"))
	num_ranks_per_channel := int(command_line_parser.IntParameter("num_ranks_per_channel"))
	num_dpus_per_rank := int(command_line_parser.IntParameter("num_dpus_per_rank"))
	this.num_dpus = num_channels * num_ranks_per_channel * num_dpus_per_rank

	this.num_tasklets = int(command_line_parser.IntParameter("num_tasklets"))

	this.Build()
}

func (this *Compiler) Build() {
	command := exec.Command("docker", "pull", "patricklidockerhub/upimulator")

	err := command.Run()

	if err != nil {
		panic(err)
	}
}

func (this *Compiler) Compile() {
	this.CompileBenchmark()
	this.CompileSdk()
}

func (this *Compiler) CompileBenchmark() {
	command := exec.Command(
		"docker",
		"run",
		"--privileged",
		"--rm",
		"-v",
		this.root_dirpath+":/root/uPIMulator",
		"patricklidockerhub/upimulator",
		"python3",
		"/root/uPIMulator/benchmark/build.py",
		"--num_dpus",
		strconv.Itoa(this.num_dpus),
		"--num_tasklets",
		strconv.Itoa(this.num_tasklets),
	)

	command.Stdout = os.Stdout
	command.Stderr = os.Stderr

	err := command.Run()

	if err != nil {
		panic(err)
	}
}

func (this *Compiler) CompileSdk() {
	command := exec.Command(
		"docker",
		"run",
		"--privileged",
		"--rm",
		"-v",
		this.root_dirpath+":/root/uPIMulator",
		"patricklidockerhub/upimulator",
		"python3",
		"/root/uPIMulator/sdk/build.py",
		"--num_tasklets",
		strconv.Itoa(this.num_tasklets),
	)

	err := command.Run()

	if err != nil {
		panic(err)
	}
}
