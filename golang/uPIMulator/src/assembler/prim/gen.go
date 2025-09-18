package prim

import (
	"bufio"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"log"
	"os"
	"uPIMulator/src/abi/encoding"
	"uPIMulator/src/abi/word"
	"uPIMulator/src/misc"
)

type Gen struct {
	num_dpus       int
	num_tasklets   int
	num_executions int

	// BFS-specific parameters matching dpu_arguments_t
	task_id []int64
	input_size [] int64
}

func (this *Gen) Init(command_line_parser *misc.CommandLineParser) {
	num_channels := int(command_line_parser.IntParameter("num_channels"))
	num_ranks_per_channel := int(command_line_parser.IntParameter("num_ranks_per_channel"))
	num_dpus_per_rank := int(command_line_parser.IntParameter("num_dpus_per_rank"))

	this.num_dpus = num_channels * num_ranks_per_channel * num_dpus_per_rank
	this.num_tasklets = int(command_line_parser.IntParameter("num_tasklets"))
	this.num_executions = 1

	// Initialize with simple constant values
	// this.num_nodes_assigned = make([]int64, this.num_dpus)
	// this.walker_container_size = make([]int64, this.num_dpus)
	// this.num_edges_assigned = make([]int64, this.num_dpus)
	this.task_id = make([]int64, this.num_executions)
	this.input_size = make([]int64, this.num_executions)

}

func (this *Gen) NumExecutions() int {
	return this.num_executions
}

func (this *Gen) InputDpuHost(execution int, dpu_id int) map[string]*encoding.ByteStream {
	if execution >= this.num_executions {
		err := errors.New("execution >= num executions")
		panic(err)
	} else if dpu_id >= this.num_dpus {
		err := errors.New("DPU ID >= num DPUs")
		panic(err)
	}

	return make(map[string]*encoding.ByteStream, 0)
}

func (this *Gen) OutputDpuHost(execution int, dpu_id int) map[string]*encoding.ByteStream {
	if execution >= this.num_executions {
		err := errors.New("execution >= num executions")
		panic(err)
	} else if dpu_id >= this.num_dpus {
		err := errors.New("DPU ID >= num DPUs")
		panic(err)
	}

	return make(map[string]*encoding.ByteStream, 0)
}

func (this *Gen) InputDpuMramHeapPointerName(execution int, dpu_id int) (int64, *encoding.ByteStream) {
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

	// if this.task_id[dpu_id] == -1 {
	// 	// If task_id is -1, return empty byte stream
	// 	fmt.Printf("Task ID for DPU %d is -1, returning empty byte stream.\n", dpu_id)
	// 	return 0, byte_stream
	// }

	filename := "Task.bin"
	fmt.Print("Task id for DPU ", execution, " is ", this.task_id[execution], " and reading from file ", filename, "\n")

	f, err := os.Open(filename)
	if err != nil {
		log.Fatal(err)
	}
	defer f.Close()


	r := bufio.NewReader(f)
	buf := make([]byte, 8)

	for {
			_, err := io.ReadFull(r, buf)
			if err == io.EOF { break }
			if err == io.ErrUnexpectedEOF { /* handle partial */ break }
			if err != nil { log.Fatal(err) }

			v := binary.LittleEndian.Uint64(buf)
			word_value := new(word.Word)
			word_value.Init(64)
			word_value.SetValue(int64(v))
			byte_stream.Merge(word_value.ToByteStream())
			// use v
	}
	return 0, byte_stream
}

func (this *Gen) OutputDpuMramHeapPointerName(execution int, dpu_id int) (int64, *encoding.ByteStream) {
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
