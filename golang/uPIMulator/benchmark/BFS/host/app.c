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

#include "../support/common.h"

// Define the DPU Binary path as DPU_BINARY here
#ifndef DPU_BINARY
#define DPU_BINARY "../dpu/BFS_device"
#endif
#define NR_DPUS 1
// Main of the Host Application
int main(int argc, char **argv) {
      struct dpu_set_t set, dpu;

    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    dpu_arguments_t input_arguments[NR_DPUS];
    for (unsigned int i = 0; i < NR_DPUS; i++) {
        input_arguments[i].num_nodes_assigned = 3; // Example value
    }

    // Copy input arrays
    int i = 0;
    DPU_FOREACH(set, dpu, i) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &input_arguments[i]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS", 0, sizeof(input_arguments[0]), DPU_XFER_DEFAULT));

    // Distribute Nodes
    DPU_FOREACH(set, dpu, i) {
        for (int j = 0; j < input_arguments[i].num_nodes_assigned; j++) {
            void * node_ptr = malloc(aligned_malloc_size(sizeof(node_t)));
            node_t node = { .id = j }; // Example node initialization
            memcpy(node_ptr, &node, sizeof(node_t));
            DPU_ASSERT(dpu_prepare_xfer(dpu, node_ptr));
            DPU_ASSERT(dpu_push_xfer(dpu, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, j * aligned_malloc_size(sizeof(node_t)), aligned_malloc_size(sizeof(node_t)), DPU_XFER_DEFAULT));
            free(node_ptr);
        }
    }


    DPU_FOREACH(set, dpu, i) {
        walker_t walker = {.visited = {0}}; // Example walker initialization
        void * walker_ptr = malloc(aligned_malloc_size(sizeof(walker_t)));
        memcpy(walker_ptr, &walker, sizeof(walker_t));
        uint32_t addr = input_arguments[i].num_nodes_assigned * aligned_malloc_size(sizeof(node_t));
        DPU_ASSERT(dpu_prepare_xfer(dpu, walker_ptr));
        DPU_ASSERT(dpu_push_xfer(dpu, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, addr, aligned_malloc_size(sizeof(walker_t)), DPU_XFER_DEFAULT));
        free(walker_ptr);
    }

    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

    DPU_FOREACH(set, dpu) {
    DPU_ASSERT(dpu_log_read(dpu, stdout));
    }
    DPU_ASSERT(dpu_free(set));
    return 0;
}
