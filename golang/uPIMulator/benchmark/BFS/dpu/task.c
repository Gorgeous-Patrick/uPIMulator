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

__host dpu_arguments_t DPU_INPUT_ARGUMENTS;

// Barrier
// BARRIER_INIT(my_barrier, NR_TASKLETS);

extern int main_kernel1(void);


// int (*kernels[nr_kernels])(void) = {main_kernel1};

int main(void) { 
    // Kernel
    // return kernels[DPU_INPUT_ARGUMENTS.kernel](); 
    return main_kernel1(); // Directly call the main_kernel1 function
}

bool find_node_by_id(uint32_t id, node_t *node) {
    // Process the walker (this is a placeholder for actual walker processing logic)
    for (uint32_t i = 0; i < DPU_INPUT_ARGUMENTS.num_nodes_assigned; i++) {
        mram_read((__mram_ptr void*)(DPU_MRAM_HEAP_POINTER + i * aligned_malloc_size(sizeof(node_t))), node, aligned_malloc_size(sizeof(node_t)));
        if (node->id == id) {
            return true;
        }
    }
    return false;
}

void container_get(uint32_t index, uint32_t *value) {
    uint32_t base = DPU_MRAM_HEAP_POINTER + DPU_INPUT_ARGUMENTS.num_nodes_assigned * aligned_malloc_size(sizeof(node_t)) + DPU_INPUT_ARGUMENTS.num_edges_assigned * aligned_malloc_size(sizeof(edge_t)) + aligned_malloc_size(sizeof(walker_t));
    mram_read((__mram_ptr void*)(base + index * aligned_malloc_size(sizeof(uint32_t))), value, aligned_malloc_size(sizeof(uint32_t)));
}

// main_kernel1
int main_kernel1() {
    unsigned int tasklet_id = me();
    printf("tasklet_id = %u\n", tasklet_id);
    if (tasklet_id == 0){ // Initialize once the cycle counter
        mem_reset(); // Reset the heap
    }
    // Barrier
    // barrier_wait(&my_barrier);


    // Get number of nodes and walkers assigned
    uint32_t num_nodes_assigned = DPU_INPUT_ARGUMENTS.num_nodes_assigned;
    uint32_t container_size = DPU_INPUT_ARGUMENTS.walker_container_size;
    uint32_t num_edges_assigned = DPU_INPUT_ARGUMENTS.num_edges_assigned;

    printf("num_nodes_assigned = %u\n", num_nodes_assigned);
    printf("container_size = %u\n", container_size);
    printf("num_edges_assigned = %u\n", num_edges_assigned);

    // Each tasklet processes one walker
    // uint32_t sum = container_get(1);
    for (uint32_t i = 0; i < num_nodes_assigned; i++) {
        node_t *node = (node_t *)mem_alloc(aligned_malloc_size(sizeof(node_t)));
        if (find_node_by_id(i, node)) {
            printf("Found node with id: %u %u\n", i, node->id);
        } else {
            printf("Node with id %u not found\n", i);
        }
    }

    for (uint32_t i = 0; i < num_edges_assigned; i++) {
        edge_t *edge = (edge_t *)mem_alloc(aligned_malloc_size(sizeof(edge_t)));
        uint32_t addr = num_nodes_assigned * aligned_malloc_size(sizeof(node_t)) + i * aligned_malloc_size(sizeof(edge_t));
        mram_read((__mram_ptr void*)(DPU_MRAM_HEAP_POINTER + addr), edge, aligned_malloc_size(sizeof(edge_t)));
        printf("Edge from %u to %u\n", edge->from, edge->to);
    }

    // Read walker struct
    walker_t *walker = (walker_t *)mem_alloc(aligned_malloc_size(sizeof(walker_t)));
    uint32_t addr = num_nodes_assigned * aligned_malloc_size(sizeof(node_t)) + num_edges_assigned * aligned_malloc_size(sizeof(edge_t));
    mram_read((__mram_ptr void*)(DPU_MRAM_HEAP_POINTER + addr), walker, aligned_malloc_size(sizeof(walker_t)));
    printf("Walker visited[0]: %u\n", walker->visited[0]);

    for (uint32_t i = 0; i < container_size; i++) {
        void *value_ptr = mem_alloc(aligned_malloc_size(sizeof(uint32_t)));

        container_get(i, value_ptr);
        printf("Container value at index %u: %u\n", i, *(uint32_t *)value_ptr);
    }
    // mram_write(&sum, (__mram_ptr void*)(DPU_MRAM_HEAP_POINTER), aligned_malloc_size(sizeof(uint32_t)));
    
    return 0;
}
