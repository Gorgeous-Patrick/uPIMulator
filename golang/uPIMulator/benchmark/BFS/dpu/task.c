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
// #define DEBUG

__host dpu_arguments_t DPU_INPUT_ARGUMENTS;

void get_node(node_t *node, uint32_t node_id) {
    // Read the node from MRAM
    uint32_t addr = DPU_MRAM_HEAP_POINTER + node_id * aligned_malloc_size(sizeof(node_t));
    mram_read((__mram_ptr void*)(addr), node, aligned_malloc_size(sizeof(node_t)));
}

void save_node(node_t *node, uint32_t node_id) {
    // Write the node back to MRAM
    uint32_t addr = DPU_MRAM_HEAP_POINTER + node_id * aligned_malloc_size(sizeof(node_t));
    mram_write(node, (__mram_ptr void*)(addr), aligned_malloc_size(sizeof(node_t)));
}

void get_walker(walker_t *walker) {
    // Read the walker from MRAM
    uint32_t addr = DPU_MRAM_HEAP_POINTER + DPU_INPUT_ARGUMENTS.num_nodes_assigned * aligned_malloc_size(sizeof(node_t));
    mram_read((__mram_ptr void*)(addr), walker, aligned_malloc_size(sizeof(walker_t)));
}

void save_walker(walker_t *walker) {
    // Write the walker back to MRAM
    uint32_t addr = DPU_MRAM_HEAP_POINTER + DPU_INPUT_ARGUMENTS.num_nodes_assigned * aligned_malloc_size(sizeof(node_t));
    mram_write(walker, (__mram_ptr void*)(addr), aligned_malloc_size(sizeof(walker_t)));
}

node_t *node_buffer;
walker_t *walker_buffer;
#define MAX_CONTAINER_BUFFER_SIZE 128
uint64_t container_buffer[MAX_CONTAINER_BUFFER_SIZE];
uint64_t container_buffer_size = 0;
void mem_init() {
    // Initialize the memory for the container value buffer, only one uint32_t.
    node_buffer = (node_t *) mem_alloc(aligned_malloc_size(sizeof(node_t)));
    walker_buffer = (walker_t *) mem_alloc(aligned_malloc_size(sizeof(walker_t)));
}

void push_new_element_to_container(uint32_t id) {
    #ifdef DEBUG
    printf("Pushing new element to container: %u\n", id);
    #endif
    if (container_buffer_size < MAX_CONTAINER_BUFFER_SIZE) {
        container_buffer[container_buffer_size++] = id;
    } else {
        #ifdef DEBUG
        printf("Container buffer overflow, cannot push new element: %u\n", id);
        #endif
    }
}

void print_container() {
    printf("Container contents: ");
    for (uint32_t i = 0; i < container_buffer_size; i++) {
        printf("%lu ", container_buffer[i]);
    }
    printf("\n");
}

void run_ability(node_t *node, uint32_t node_id, walker_t *walker) {
    // Placeholder for ability logic
    #ifdef DEBUG
    printf("Running ability for node %u\n", node->id);
    printf("visited: %u\n", walker->visited[node->id]);
    #endif
    if (walker->visited[node->id] == 0) {
        walker->visited[node->id] = 1;
        if (node_id == 0) {
            push_new_element_to_container(1);
            push_new_element_to_container(2);
            push_new_element_to_container(3);
        } else if (node_id == 1) {
            push_new_element_to_container(4);
            push_new_element_to_container(5);
        } else if (node_id == 2) {
            push_new_element_to_container(6);
        }
    }
}

extern int main_kernel1(void);




int main(void) { 
    // Kernel
    // return kernels[DPU_INPUT_ARGUMENTS.kernel](); 
    // Initialize memory
    unsigned int tasklet_id = me();

    #ifdef DEBUG
    printf("tasklet_id = %u\n", tasklet_id);
    #endif
    if (tasklet_id == 0){ // Initialize once the cycle counter
        mem_reset(); // Reset the heap
    }
    mem_init();
    return main_kernel1(); // Directly call the main_kernel1 function
}

int main_kernel1() {
    // Barrier
    // barrier_wait(&my_barrier);


    // Get number of nodes and walkers assigned
    uint32_t num_nodes_assigned = DPU_INPUT_ARGUMENTS.num_nodes_assigned;

    #ifdef DEBUG
    printf("num_nodes_assigned = %u\n", num_nodes_assigned);
    #endif


    int cnt = 0;
    get_walker(walker_buffer);

    get_node(node_buffer, 0);
    run_ability(node_buffer, 0, walker_buffer);
    save_node(node_buffer, 0);

    get_node(node_buffer, 1);
    run_ability(node_buffer, 1, walker_buffer);
    save_node(node_buffer, 1);

    get_node(node_buffer, 2);
    run_ability(node_buffer, 2, walker_buffer);
    save_node(node_buffer, 2);
        

    save_walker(walker_buffer);
    #ifdef DEBUG
    printf("Ending.\n");
    print_container();
    #endif
    // mram_write(&sum, (__mram_ptr void*)(DPU_MRAM_HEAP_POINTER), aligned_malloc_size(sizeof(uint32_t)));
    return 0;
}
