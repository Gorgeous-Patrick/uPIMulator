/*
* Vector addition with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>

#include "../support/common.h"

typedef struct {
    uint32_t id;
} node_t;

typedef struct {
    uint32_t from;
    uint32_t to;
    uint32_t type;
} edge_t;

__host dpu_arguments_t DPU_INPUT_ARGUMENTS;

// Barrier
BARRIER_INIT(my_barrier, NR_TASKLETS);

extern int main_kernel1(void);


// int (*kernels[nr_kernels])(void) = {main_kernel1};

int main(void) { 
    // Kernel
    // return kernels[DPU_INPUT_ARGUMENTS.kernel](); 
    return main_kernel1(); // Directly call the main_kernel1 function
}

uint32_t aligned_malloc_size(uint32_t size) {
    // Align the size to 8 bytes
    return (size + 7) & ~7;
}

bool find_node_by_id(uint32_t id, node_t *node) {
    // Process the walker (this is a placeholder for actual walker processing logic)
    for (uint32_t i = 0; i < DPU_INPUT_ARGUMENTS.num_nodes_assigned; i++) {
        mram_read((__mram_ptr void*)(DPU_MRAM_HEAP_POINTER + i * sizeof(node_t)), node, aligned_malloc_size(sizeof(node_t)));
        if (node->id == id) {
            return true;
        }
    }
    return false;
}

uint32_t container_get(uint32_t index) {
    uint64_t value;
    mram_read((__mram_ptr void*)(DPU_MRAM_HEAP_POINTER + DPU_INPUT_ARGUMENTS.num_nodes_assigned * sizeof(node_t) + index * sizeof(uint32_t)), &value, aligned_malloc_size(sizeof(uint32_t)));
    return value;
}

// main_kernel1
int main_kernel1() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    if (tasklet_id == 0){ // Initialize once the cycle counter
        mem_reset(); // Reset the heap
    }
    // Barrier
    barrier_wait(&my_barrier);


    // Get number of nodes and walkers assigned
    uint32_t num_nodes_assigned = DPU_INPUT_ARGUMENTS.num_nodes_assigned;
    uint32_t container_size = DPU_INPUT_ARGUMENTS.walker_container_size;
    uint32_t num_edges_assigned = DPU_INPUT_ARGUMENTS.num_edges_assigned;

    // Each tasklet processes one walker
    uint32_t sum = 0;
    for (uint32_t i = 0; i < container_size; i++) {
        uint32_t node_id = container_get(i);
        
        // Find the node by id
        // node_t node;
        node_t *node = (node_t *)mem_alloc(aligned_malloc_size(sizeof(node_t)));
        if (find_node_by_id(node_id, node)) {
            // If found, add the node id to the sum
            sum += node->id;
        }
    }
    mram_write(&sum, (__mram_ptr void*)(DPU_MRAM_HEAP_POINTER), aligned_malloc_size(sizeof(uint32_t)));
    
    return 0;
}
