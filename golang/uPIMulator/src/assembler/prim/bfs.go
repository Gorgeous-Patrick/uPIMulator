package prim

import (
	"errors"
	"uPIMulator/src/abi/encoding"
	"uPIMulator/src/abi/word"
	"uPIMulator/src/misc"
)

type Bfs struct {
	num_dpus       int
	num_tasklets   int
	num_executions int

	// BFS-specific parameters matching dpu_arguments_t
	num_nodes_assigned    []int64
	walker_container_size []int64
	num_edges_assigned    []int64
}

func (this *Bfs) Init(command_line_parser *misc.CommandLineParser) {
	num_channels := int(command_line_parser.IntParameter("num_channels"))
	num_ranks_per_channel := int(command_line_parser.IntParameter("num_ranks_per_channel"))
	num_dpus_per_rank := int(command_line_parser.IntParameter("num_dpus_per_rank"))

	this.num_dpus = num_channels * num_ranks_per_channel * num_dpus_per_rank
	this.num_tasklets = int(command_line_parser.IntParameter("num_tasklets"))
	this.num_executions = 1

	// Initialize with simple constant values
	this.num_nodes_assigned = make([]int64, this.num_dpus)
	this.walker_container_size = make([]int64, this.num_dpus)
	this.num_edges_assigned = make([]int64, this.num_dpus)

	for i := 0; i < this.num_dpus; i++ {
		this.num_nodes_assigned[i] = 50     // Simple constant
		this.walker_container_size[i] = 128 // Simple constant
		this.num_edges_assigned[i] = 200    // Simple constant
	}
}

func (this *Bfs) NumExecutions() int {
	return this.num_executions
}

func (this *Bfs) InputDpuHost(execution int, dpu_id int) map[string]*encoding.ByteStream {
	if execution >= this.num_executions {
		err := errors.New("execution >= num executions")
		panic(err)
	} else if dpu_id >= this.num_dpus {
		err := errors.New("DPU ID >= num DPUs")
		panic(err)
	}

	dpu_input_arguments_byte_stream := new(encoding.ByteStream)
	dpu_input_arguments_byte_stream.Init()

	// Send num_nodes_assigned (constant value 50)
	num_nodes_assigned_word := new(word.Word)
	num_nodes_assigned_word.Init(32)
	num_nodes_assigned_word.SetValue(50)
	dpu_input_arguments_byte_stream.Merge(num_nodes_assigned_word.ToByteStream())

	// Send walker_container_size (constant value 128)
	walker_container_size_word := new(word.Word)
	walker_container_size_word.Init(32)
	walker_container_size_word.SetValue(128)
	dpu_input_arguments_byte_stream.Merge(walker_container_size_word.ToByteStream())

	// Send num_edges_assigned (constant value 200)
	num_edges_assigned_word := new(word.Word)
	num_edges_assigned_word.Init(32)
	num_edges_assigned_word.SetValue(200)
	dpu_input_arguments_byte_stream.Merge(num_edges_assigned_word.ToByteStream())

	dpu_host := make(map[string]*encoding.ByteStream, 0)
	dpu_host["DPU_INPUT_ARGUMENTS"] = dpu_input_arguments_byte_stream

	return dpu_host
}

func (this *Bfs) OutputDpuHost(execution int, dpu_id int) map[string]*encoding.ByteStream {
	if execution >= this.num_executions {
		err := errors.New("execution >= num executions")
		panic(err)
	} else if dpu_id >= this.num_dpus {
		err := errors.New("DPU ID >= num DPUs")
		panic(err)
	}

	return make(map[string]*encoding.ByteStream, 0)
}

func (this *Bfs) InputDpuMramHeapPointerName(execution int, dpu_id int) (int64, *encoding.ByteStream) {
	if execution >= this.num_executions {
		err := errors.New("execution >= num executions")
		panic(err)
	} else if dpu_id >= this.num_dpus {
		err := errors.New("DPU ID >= num DPUs")
		panic(err)
	}

	// Return empty stream - no MRAM input needed
	byte_stream := new(encoding.ByteStream)
	byte_stream.Init()

	return 0, byte_stream
}

func (this *Bfs) OutputDpuMramHeapPointerName(execution int, dpu_id int) (int64, *encoding.ByteStream) {
	if execution >= this.num_executions {
		err := errors.New("execution >= num executions")
		panic(err)
	} else if dpu_id >= this.num_dpus {
		err := errors.New("DPU ID >= num DPUs")
		panic(err)
	}

	// Create expected output - sum of the three DPU_INPUT_ARGUMENTS values
	byte_stream := new(encoding.ByteStream)
	byte_stream.Init()

	// Expected sum: 50 + 128 + 200 = 378
	sum_word := new(word.Word)
	sum_word.Init(32)
	sum_word.SetValue(378) // Sum of num_nodes_assigned + walker_container_size + num_edges_assigned
	byte_stream.Merge(sum_word.ToByteStream())

	// Return offset 0 since no input data in MRAM
	return 0, byte_stream
}
