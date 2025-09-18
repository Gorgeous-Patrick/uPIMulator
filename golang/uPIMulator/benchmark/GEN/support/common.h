#ifndef _COMMON_H_
#define _COMMON_H_

// Structures used by both the host and the dpu to communicate information
typedef struct {
    uint32_t num_nodes_assigned;
} dpu_arguments_t;

typedef struct {
    uint32_t id;
} node_t;

typedef struct {
	  uint32_t visited[32]; // Example size
} walker_t;

uint32_t aligned_malloc_size(uint32_t size) {
    // Align the size to 8 bytes
    return (size + 7) & ~7;
}
#endif
