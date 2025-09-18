#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>

// #define DEBUG

void get(void * buf, uint32_t start, uint32_t size) {
    // Read the node from MRAM
    uint32_t addr = DPU_MRAM_HEAP_POINTER + start;
    mram_read((__mram_ptr void*)(addr), buf, size);
}

void save(void * buf, uint32_t start, uint32_t size) {
    // Write the walker back to MRAM
    uint32_t addr = DPU_MRAM_HEAP_POINTER + start;
    mram_write(buf, (__mram_ptr void*)(addr), size);

}

#define MAX_CONTAINER_BUFFER_SIZE 128
uint64_t container_buffer[MAX_CONTAINER_BUFFER_SIZE];
uint64_t container_buffer_size = 0;

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

typedef struct __BranchNode {
uint64_t mid;
} BranchNode;

typedef struct __DataNode {
uint64_t value; uint64_t index;
} DataNode;



typedef struct __bs {
uint64_t value;
} bs;


void printnode_bs_DataNode (DataNode *node, uint32_t node_id, bs* walker) {

  
}

void rundown_bs_BranchNode (BranchNode *node, uint32_t node_id, bs* walker) {

    if (walker->value < node->mid) {
      push_new_element_to_container(0);
    } else {
      push_new_element_to_container(1);
    }
  
}


void *node_buffer;
void *walker_buffer;
inline void mem_init() {
    node_buffer = mem_alloc(16);
    walker_buffer = mem_alloc(8);
}

int main() { 
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
    get(walker_buffer, 472, 8);
    
    get(node_buffer, 0, 8); 
    rundown_bs_BranchNode(node_buffer, 1, walker_buffer);
    save(node_buffer, 0, 8); 
    
    save(walker_buffer, 472, 8);

    #ifdef DEBUG
    printf("Ending.\n");
    print_container();
    #endif
    // mram_write(&sum, (__mram_ptr void*)(DPU_MRAM_HEAP_POINTER), aligned_malloc_size(sizeof(uint32_t)));
    return 0;
}