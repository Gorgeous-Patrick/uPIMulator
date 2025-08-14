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
		this.num_nodes_assigned[i] = 50   // Simple constant
		this.walker_container_size[i] = 2 // Simple constant
		this.num_edges_assigned[i] = 200  // Simple constant
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
	num_nodes_assigned_word.SetValue(this.num_nodes_assigned[dpu_id]) // Send actual num_nodes_assigned (50)
	dpu_input_arguments_byte_stream.Merge(num_nodes_assigned_word.ToByteStream())

	// Send walker_container_size (constant value 2)
	walker_container_size_word := new(word.Word)
	walker_container_size_word.Init(32)
	walker_container_size_word.SetValue(this.walker_container_size[dpu_id]) // Send actual walker_container_size (2)
	dpu_input_arguments_byte_stream.Merge(walker_container_size_word.ToByteStream())

	// Send num_edges_assigned (constant value 200)
	num_edges_assigned_word := new(word.Word)
	num_edges_assigned_word.Init(32)
	num_edges_assigned_word.SetValue(this.num_edges_assigned[dpu_id]) // Send actual num_edges_assigned (200)
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

	// Create node data for MRAM
	byte_stream := new(encoding.ByteStream)
	byte_stream.Init()

	// The task.c should read num_nodes_assigned (50) nodes, each with an ID
	num_nodes := this.num_nodes_assigned[dpu_id]
	num_edges := this.num_edges_assigned[dpu_id]

	for i := int64(0); i < num_nodes; i++ {
		// Each node_t has a uint32_t id field
		node_id_word := new(word.Word)
		node_id_word.Init(32)
		node_id_word.SetValue(i) // IDs from 0 to 49
		byte_stream.Merge(node_id_word.ToByteStream())
	}
	for i := int64(0); i < num_edges; i++ {
		// Each edge_t has three uint32_t id fields (from to type).
		from_node_id_word := new(word.Word)
		from_node_id_word.Init(32)
		from_node_id_word.SetValue(i % num_nodes)
		byte_stream.Merge(from_node_id_word.ToByteStream())

		to_node_id_word := new(word.Word)
		to_node_id_word.Init(32)
		to_node_id_word.SetValue((i + 1) % num_nodes)
		byte_stream.Merge(to_node_id_word.ToByteStream())

		edge_type_word := new(word.Word)
		edge_type_word.Init(32)
		edge_type_word.SetValue(0) // Placeholder for edge type
		byte_stream.Merge(edge_type_word.ToByteStream())

		dummy_word := new(word.Word)
		dummy_word.Init(32)
		dummy_word.SetValue(0) // Placeholder for edge type
		byte_stream.Merge(dummy_word.ToByteStream())
	}

	// Add walker struct: walker_t with bool visited[10]
	// Each bool takes 1 byte, but we'll align to 32-bit words for simplicity
	for i := 0; i < 10; i++ {
		visited_word := new(word.Word)
		visited_word.Init(8)     // 8 bits for bool
		visited_word.SetValue(0) // Initialize all visited flags to false
		byte_stream.Merge(visited_word.ToByteStream())
	}

	// Add padding to align to 32-bit boundary (10 bytes + 6 padding = 16 bytes)
	for i := 0; i < 6; i++ {
		padding_byte := new(word.Word)
		padding_byte.Init(8)
		padding_byte.SetValue(0)
		byte_stream.Merge(padding_byte.ToByteStream())
	}

	// Walker container: uint32_t array of node ids (container_size = 2)
	container_1 := new(word.Word)
	container_1.Init(32)
	container_1.SetValue(1) // Placeholder for container ID
	byte_stream.Merge(container_1.ToByteStream())

	container_2 := new(word.Word)
	container_2.Init(32)
	container_2.SetValue(1) // Placeholder for container ID
	byte_stream.Merge(container_2.ToByteStream())

	// for i := int64(0); i < this.walker_container_size[dpu_id]; i++ {
	// 	// Each container element is a uint32_t id field
	// 	walker_id_word := new(word.Word)
	// 	walker_id_word.Init(32)
	// 	walker_id_word.SetValue(i) // IDs from 0 to 127
	// 	byte_stream.Merge(walker_id_word.ToByteStream())
	// }

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

	// Create expected output - task.c writes container_size (not sum) to MRAM
	byte_stream := new(encoding.ByteStream)
	byte_stream.Init()

	// The task.c writes container_size (which is 2) to MRAM
	sum_word := new(word.Word)
	sum_word.Init(32)
	sum_word.SetValue(0) // container_size value from DPU_INPUT_ARGUMENTS
	byte_stream.Merge(sum_word.ToByteStream())

	// Return offset 0 - DPU writes result at the beginning of MRAM
	return 0, byte_stream
}
