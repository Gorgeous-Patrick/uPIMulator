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

// #define DEBUG

// Barrier
// BARRIER_INIT(my_barrier, NR_TASKLETS);

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

void get_walker(walker_t *walker) {
    // Read the walker from MRAM
    uint32_t addr = DPU_MRAM_HEAP_POINTER + DPU_INPUT_ARGUMENTS.num_nodes_assigned * aligned_malloc_size(sizeof(node_t)) + DPU_INPUT_ARGUMENTS.num_edges_assigned * aligned_malloc_size(sizeof(edge_t));
    mram_read((__mram_ptr void*)(addr), walker, aligned_malloc_size(sizeof(walker_t)));
}

void save_walker(walker_t *walker) {
    // Write the walker back to MRAM
    uint32_t addr = DPU_MRAM_HEAP_POINTER + DPU_INPUT_ARGUMENTS.num_nodes_assigned * aligned_malloc_size(sizeof(node_t)) + DPU_INPUT_ARGUMENTS.num_edges_assigned * aligned_malloc_size(sizeof(edge_t));
    mram_write(walker, (__mram_ptr void*)(addr), aligned_malloc_size(sizeof(walker_t)));
}

uint32_t *container_value_buffer;
node_t *node_buffer;
walker_t *walker_buffer;
void mem_init() {
    // Initialize the memory for the container value buffer, only one uint32_t.
    container_value_buffer = (uint32_t *) mem_alloc(aligned_malloc_size(sizeof(uint32_t)));
    node_buffer = (node_t *) mem_alloc(aligned_malloc_size(sizeof(node_t)));
    walker_buffer = (walker_t *) mem_alloc(aligned_malloc_size(sizeof(walker_t)));
}

void print_container() {
    for (uint32_t i = 0; i < DPU_INPUT_ARGUMENTS.walker_container_size; i++) {
        container_get(i, container_value_buffer);
        printf("Container value at index %u: %u\n", i, *container_value_buffer);
    }
}

void push_new_elements_to_container(uint32_t id) {
    for (uint32_t i = 0; i < DPU_INPUT_ARGUMENTS.num_edges_assigned; i++) {
        edge_t *edge = (edge_t *)mem_alloc(aligned_malloc_size(sizeof(edge_t)));
        uint32_t addr = DPU_INPUT_ARGUMENTS.num_nodes_assigned * aligned_malloc_size(sizeof(node_t)) + i * aligned_malloc_size(sizeof(edge_t));
        mram_read((__mram_ptr void*)(DPU_MRAM_HEAP_POINTER + addr), edge, aligned_malloc_size(sizeof(edge_t)));
        if (edge->from != id) {
            continue;
        }
        #ifdef DEBUG
        printf("Pushing edge from %u to %u\n", edge->from, edge->to);
        #endif
        // Push the edge to the container
        uint32_t value = edge->to;
        uint32_t base = DPU_MRAM_HEAP_POINTER + DPU_INPUT_ARGUMENTS.num_nodes_assigned * aligned_malloc_size(sizeof(node_t)) + DPU_INPUT_ARGUMENTS.num_edges_assigned * aligned_malloc_size(sizeof(edge_t)) + aligned_malloc_size(sizeof(walker_t));
        mram_write(&value, (__mram_ptr void*)base + DPU_INPUT_ARGUMENTS.walker_container_size * aligned_malloc_size(sizeof(uint32_t)), aligned_malloc_size(sizeof(uint32_t)));
        // Update the walker container size
        DPU_INPUT_ARGUMENTS.walker_container_size++;
        #ifdef DEBUG
        printf("New walker container size after push: %u\n", DPU_INPUT_ARGUMENTS.walker_container_size);
        print_container();
        #endif
    }
}

void pop_element_from_container() {
    // Pop the first element from the container
    if (DPU_INPUT_ARGUMENTS.walker_container_size == 0) {
        #ifdef DEBUG
        printf("Container is empty, nothing to pop.\n");
        #endif
        return;
    }
    
    uint32_t base = DPU_MRAM_HEAP_POINTER + DPU_INPUT_ARGUMENTS.num_nodes_assigned * aligned_malloc_size(sizeof(node_t)) + DPU_INPUT_ARGUMENTS.num_edges_assigned * aligned_malloc_size(sizeof(edge_t)) + aligned_malloc_size(sizeof(walker_t));
    #ifdef DEBUG
    mram_read((__mram_ptr void*)(base), container_value_buffer, aligned_malloc_size(sizeof(uint32_t)));
    printf("Popping value: %u\n", *container_value_buffer);
    #endif
    // Shift the remaining elements in the container
    for (uint32_t i = 1; i < DPU_INPUT_ARGUMENTS.walker_container_size; i++) {
        mram_read((__mram_ptr void*)(base + i * aligned_malloc_size(sizeof(uint32_t))), container_value_buffer, aligned_malloc_size(sizeof(uint32_t)));
        mram_write(container_value_buffer, (__mram_ptr void*)(base + (i - 1) * aligned_malloc_size(sizeof(uint32_t))), aligned_malloc_size(sizeof(uint32_t)));
    }
    DPU_INPUT_ARGUMENTS.walker_container_size--;
    #ifdef DEBUG
    printf("New walker container size after pop: %u\n", DPU_INPUT_ARGUMENTS.walker_container_size);
    print_container();
    #endif
}

void run_ability(node_t *node, walker_t *walker) {
    // Placeholder for ability logic
    #ifdef DEBUG
    printf("Running ability for node %u\n", node->id);
    printf("visited: %u\n", walker->visited[node->id]);
    #endif
    if (walker->visited[node->id] == 0) {
        walker->visited[node->id] = 1;
        push_new_elements_to_container(node->id);
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
    uint32_t container_size = DPU_INPUT_ARGUMENTS.walker_container_size;
    uint32_t num_edges_assigned = DPU_INPUT_ARGUMENTS.num_edges_assigned;

    #ifdef DEBUG
    printf("num_nodes_assigned = %u\n", num_nodes_assigned);
    printf("container_size = %u\n", container_size);
    printf("num_edges_assigned = %u\n", num_edges_assigned);
    #endif

    // // Each tasklet processes one walker
    // // uint32_t sum = container_get(1);
    // for (uint32_t i = 0; i < num_nodes_assigned; i++) {
    //     node_t *node = (node_t *)mem_alloc(aligned_malloc_size(sizeof(node_t)));
    //     if (find_node_by_id(i, node)) {
    //         printf("Found node with id: %u %u\n", i, node->id);
    //     } else {
    //         printf("Node with id %u not found\n", i);
    //     }
    // }

    int cnt = 0;
    get_walker(walker_buffer);
    
    while (DPU_INPUT_ARGUMENTS.walker_container_size > 0) {
        // Get the first node from the container
        container_get(0, container_value_buffer);
        if (find_node_by_id(*container_value_buffer, node_buffer)) {
            #ifdef DEBUG
            printf("Node found in container: %u\n", node_buffer->id);
            #endif
            pop_element_from_container();
            run_ability(node_buffer, walker_buffer);
        } else {
            #ifdef DEBUG
            printf("Node with id %u not found in container\n", *container_value_buffer);
            #endif
            break;
        }
    }

    save_walker(walker_buffer);
    #ifdef DEBUG
    printf("Ending.\n");
    print_container();
    #endif
    // mram_write(&sum, (__mram_ptr void*)(DPU_MRAM_HEAP_POINTER), aligned_malloc_size(sizeof(uint32_t)));
    return 0;
}
