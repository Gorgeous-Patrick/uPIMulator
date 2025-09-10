/**
* app.c
* VA Host Application Source File
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dpu.h>
#include <dpu_log.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <dpu_types.h>

#include "file_read.h"

#include "../support/common.h"

// Define the DPU Binary path as DPU_BINARY here
#ifndef DPU_BINARY
#define DPU_BINARY "../dpu/BFS_device"
#endif
#define NR_DPUS 1
#define N 3
// Main of the Host Application
int main(int argc, char **argv) {
      struct dpu_set_t set, dpu;

    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    uint64_t task_id = 0;
    while (task_id < N) {
        DPU_FOREACH(set, dpu) {
            if (task_id >= N) {
                break;
            }
            printf("Task ID: %llu\n", (unsigned long long)task_id);
            DPU_ASSERT(dpu_prepare_xfer(dpu, &task_id));
            DPU_ASSERT(dpu_push_xfer(dpu, DPU_XFER_TO_DPU, "task_id", 0, sizeof(uint64_t), DPU_XFER_DEFAULT));
            char fname[32];
            snprintf(fname, sizeof(fname), "task_bins/Task%llu.bin", (unsigned long long)task_id);

            xfer_file_to_dpu(dpu, fname, 0, NULL);
            task_id++;
        }
        DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
        DPU_FOREACH(set, dpu) {
        DPU_ASSERT(dpu_log_read(dpu, stdout));
        }
    }


    DPU_ASSERT(dpu_free(set));
    return 0;
}
