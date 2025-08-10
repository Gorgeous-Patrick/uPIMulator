package prim

import (
	"uPIMulator/src/abi/encoding"
	"uPIMulator/src/misc"
)

type Bfs struct {
	va Va // Just wrap the VA implementation
}

func (this *Bfs) Init(command_line_parser *misc.CommandLineParser) {
	this.va.Init(command_line_parser)
}

func (this *Bfs) NumExecutions() int {
	return this.va.NumExecutions()
}

func (this *Bfs) InputDpuHost(execution int, dpu_id int) map[string]*encoding.ByteStream {
	return this.va.InputDpuHost(execution, dpu_id)
}

func (this *Bfs) OutputDpuHost(execution int, dpu_id int) map[string]*encoding.ByteStream {
	return this.va.OutputDpuHost(execution, dpu_id)
}

func (this *Bfs) InputDpuMramHeapPointerName(execution int, dpu_id int) (int64, *encoding.ByteStream) {
	return this.va.InputDpuMramHeapPointerName(execution, dpu_id)
}

func (this *Bfs) OutputDpuMramHeapPointerName(execution int, dpu_id int) (int64, *encoding.ByteStream) {
	return this.va.OutputDpuMramHeapPointerName(execution, dpu_id)
}
