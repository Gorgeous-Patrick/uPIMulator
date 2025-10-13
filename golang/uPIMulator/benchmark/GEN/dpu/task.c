#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <barrier.h>

// #define DEBUG
// typedef struct __Metadata {
//   uint64_t extra_mram_space; uint64_t walker_num; uint64_t walker_container_ptrs[12]; uint64_t trace_lengths[12];
// } Metadata;



// typedef struct __ContainerObject{
// uint64_t walker_ptr; uint64_t walker_size; uint64_t node_ptr; uint64_t node_size; uint64_t edge_num; uint64_t func_call;
// } ContainerObject;



// typedef struct __BranchNode {
// uint64_t mid;
// } BranchNode;

// typedef struct __DataNode {
// uint64_t value; uint64_t index;
// } DataNode;



// typedef struct __bs {
// uint64_t value;
// } bs;


// #define MAX_CONTAINER_BUFFER_SIZE 128
// uint64_t container_buffer[MAX_CONTAINER_BUFFER_SIZE];
// uint64_t container_buffer_size = 0;
// void push_new_element_to_container(uint32_t id) {
//     #ifdef DEBUG
//     printf("Pushing new element to container: %u\n", id);
//     #endif
//     if (container_buffer_size < MAX_CONTAINER_BUFFER_SIZE) {
//         container_buffer[0] = id;
//     } else {
//         #ifdef DEBUG
//         printf("Container buffer overflow, cannot push new element: %u\n", id);
//         #endif
//     }
// }


// void printnode_bs_DataNode (DataNode *node, uint32_t node_id, bs* walker) {

//   #ifdef DEBUG
//   printf("DPU Tasklet %u: DataNode - Value: %lu, Index: %lu\n", me(), node->value, node->index);
//   #endif

  
// }

// void rundown_bs_BranchNode (BranchNode *node, uint32_t node_id, bs* walker) {

//   #ifdef DEBUG
//   printf("DPU Tasklet %u: BranchNode - Mid: %lu\n", me(), node->mid);
//   #endif

//     if (walker->value < node->mid) {
//       push_new_element_to_container(0);
//     } else {
//       push_new_element_to_container(1);
//     }
  
// }


// void run_on_node(uint64_t walker_ptr, uint64_t node_ptr, uint64_t edge_num, uint64_t func_call) {
  
//   if (func_call == 0) {
//     printnode_bs_DataNode(node_ptr, 0, walker_ptr);
//   }
  
//   if (func_call == 1) {
//     rundown_bs_BranchNode(node_ptr, 0, walker_ptr);
//   }
  
// }
// void get(void * buf, uint32_t start, uint32_t size) {
//     // Read the node from MRAM
//     uint32_t addr = DPU_MRAM_HEAP_POINTER + start;
//     mram_read((__mram_ptr void*)(addr), buf, size);
// }

// void save(void * buf, uint32_t start, uint32_t size) {
//     // Write the walker back to MRAM
//     uint32_t addr = DPU_MRAM_HEAP_POINTER + start;
//     mram_write(buf, (__mram_ptr void*)(addr), size);
// }


// void run_thread(uint64_t walker_container_ptr, uint64_t trace_length, char * node_buffer, char * walker_buffer) {
//     ContainerObject container_obj;
//     for (uint64_t i = 0; i < trace_length; i++) {
//         get(&container_obj, walker_container_ptr + i * sizeof(ContainerObject), sizeof(ContainerObject));
//         #ifdef DEBUG
//         printf("DPU Tasklet %u: Container Object - Walker ptr: %lu, Walker size: %lu, Node ptr: %lu, Node size: %lu, Edge num: %lu, Func call: %lu\n", me(), container_obj.walker_ptr, container_obj.walker_size, container_obj.node_ptr, container_obj.node_size, container_obj.edge_num, container_obj.func_call);
//         #endif
//         // Load node
//         get(node_buffer, container_obj.node_ptr, container_obj.node_size);
//         // Load walker
//         get(walker_buffer, container_obj.walker_ptr, container_obj.walker_size);
//         // Run on node
//         run_on_node(walker_buffer, node_buffer, container_obj.edge_num, container_obj.func_call);
//         // Save walker
//         save(walker_buffer, container_obj.walker_ptr, container_obj.walker_size);
//         // Save node
//         save(node_buffer, container_obj.node_ptr, container_obj.node_size);
//     }
// }
BARRIER_INIT(my_barrier, NR_TASKLETS);

// __host uint64_t my_dpu_id;
uint64_t m_dpu_id = 0;

int main() {
  // Do some calculation proportional to the DPU id
  uint64_t result = 0;
  uint64_t rand[5] = {123456789, 362436069, 521288629, 88675123, 5783321};

  if (me() == 0) {
    // my_dpu_id = *((uint64_t*)__mram_ptr void const*)DPU_MRAM_HEAP_POINTER;
    // #ifdef DEBUG
    // printf("DPU Tasklet %u: DPU id from MRAM: %lu\n", me(), my_dpu_id);
    mem_reset();
    mram_read((__mram_ptr void const*)DPU_MRAM_HEAP_POINTER, &m_dpu_id, 8);
    // #endif
  }
  // BARRIER_WAIT(&my_barrier);


  for (uint64_t i = 0; i < rand[m_dpu_id % 5] % 10; i++) {
    result += i;
  }
  // printf("DPU Tasklet %u: Finished processing for DPU id %lu, result: %lu\n", me(), dpu_id, result);
  
  // dpu_id = result;
  mram_write(&result, (__mram_ptr void*)DPU_MRAM_HEAP_POINTER, sizeof(uint64_t));
  return 0;
}