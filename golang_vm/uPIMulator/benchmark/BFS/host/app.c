#include <stdlib.h>
#include <dpu.h>
#define NODE_NUM 100
#define NAME_SIZE 30
#define MAX_NODE_NUM_PER_DPU 8
#define NUM_DPU 20
#define MAX_OUTPUT 1000
#define MAX_EDGE_NUM_PER_DPU 1000

struct node_t {
  int id;
  int kernel;
};

struct walker_impl_t {
  int container_size;
};

struct edge_impl_t {
  int from;
  int to;
};

struct metadata_t {
  int node_num;
  int edge_num;
};

void set_data_mapping(int * data_mapping) {
    for (int i = 0; i < NODE_NUM; i++) {
        data_mapping[i] = i % NUM_DPU;
    }
}

void read_input(int * matrix, struct node_t* nodes) {
    for (int i = 0; i < NODE_NUM * NODE_NUM; i++) {
        if (i % 2 == 1) {
            matrix[i] = 1;
        } else {
            matrix[i] = -1;
        }
    }

    for (int i = 0; i < NODE_NUM; i++) {
        nodes[i].id = i;
        nodes[i].kernel = 0;
    }
}

void distribute_node_metadata(struct dpu_set_t dpu_set, int * data_mapping) {
    int i = 0;
    struct dpu_set_t dpu;
    int * cnt = malloc(sizeof(int) * NUM_DPU);
    for (int j = 0; j < NUM_DPU; j++) {
        cnt[j] = 0;
    }
    DPU_FOREACH(dpu_set, dpu, i) {
        for (int j = 0; j < NODE_NUM; j++) {
            if (data_mapping[j] == i) {
                cnt[i] = cnt[i] + 1;
            }
        }
        dpu_prepare_xfer(dpu, &cnt[i]);
    }
    dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_CNT", 0, sizeof(int), DPU_XFER_DEFAULT);
    free(cnt);
}

void distribute_nodes(struct dpu_set_t dpu_set, struct node_t * nodes, int * data_mapping) {
    int i = 0;
    struct dpu_set_t dpu;
    int * nodes_list = malloc(sizeof(int) * MAX_NODE_NUM_PER_DPU * NUM_DPU);
    for (int j = 0; j < MAX_NODE_NUM_PER_DPU * NUM_DPU; j++) {
        nodes_list[j] = -1;
    }
    DPU_FOREACH(dpu_set, dpu, i) {
        int cnt = 0;
        for (int j = 0; j < NODE_NUM; j++) {
            if (data_mapping[j] == i) {
                int idx = i * MAX_NODE_NUM_PER_DPU + cnt;
                nodes_list[idx] = nodes[j].id;
                printf("Node %d: %d, kernel: %d\n", j, nodes_list[idx], nodes[j].kernel);
                cnt++;
            }
        }
        dpu_prepare_xfer(dpu, &nodes_list[i * MAX_NODE_NUM_PER_DPU]);
        dpu_push_xfer(dpu, DPU_XFER_TO_DPU, "NODES", 0, sizeof(int) * MAX_NODE_NUM_PER_DPU, DPU_XFER_DEFAULT);
    }
    for (int j = 0; j < NUM_DPU; j++) {
        for (int k = 0; k < MAX_NODE_NUM_PER_DPU; k++) {
            printf("Node %d: %d\n", j * MAX_NODE_NUM_PER_DPU + k, nodes_list[j * MAX_NODE_NUM_PER_DPU + k]);
        }
    }
    free(nodes_list);
}

void distribute_edges(struct dpu_set_t dpu_set, int * matrix, int * data_mapping) {
    int i = 0;
    struct dpu_set_t dpu;
    int * edges_list = malloc(sizeof(int) * MAX_EDGE_NUM_PER_DPU * NUM_DPU);
    for (int j = 0; j < MAX_EDGE_NUM_PER_DPU * NUM_DPU; j++) {
        edges_list[j] = -1;
    }
    DPU_FOREACH(dpu_set, dpu, i) {
        int cnt = 0;
        for (int j = 0; j < NODE_NUM; j++) {
            if (data_mapping[j] == i) {
                for (int k = 0; k < NODE_NUM; k++) {
                    if (matrix[j * NODE_NUM + k] == 1) {
                        int idx = i * MAX_EDGE_NUM_PER_DPU + cnt;
                        edges_list[idx] = k;
                        cnt++;
                    }
                }
            }
        }
        dpu_prepare_xfer(dpu, &edges_list[i * MAX_EDGE_NUM_PER_DPU]);
        dpu_push_xfer(dpu, DPU_XFER_TO_DPU, "EDGES", 0, sizeof(int) * MAX_EDGE_NUM_PER_DPU, DPU_XFER_DEFAULT);
    }
    free(edges_list);
}

void distribute(struct dpu_set_t dpu_set, int * matrix, struct node_t * nodes, int * data_mapping) {
    distribute_node_metadata(dpu_set, data_mapping);
    distribute_nodes(dpu_set, nodes, data_mapping);
    distribute_edges(dpu_set, matrix, data_mapping);
}


int main() {
    int* container = malloc(sizeof(int) * NODE_NUM + 1);
    int* matrix = malloc(sizeof(int) * NODE_NUM * NODE_NUM);
    struct node_t * nodes = malloc(sizeof(struct node_t) * NODE_NUM);
    for (int i = 0; i < NODE_NUM + 1; i++) {
        container[i] = -1;
    }
    container[0] = 0;
    read_input(matrix, nodes);
    
    struct dpu_set_t dpu_set;
    struct dpu_set_t dpu;

    int nr_of_dpus = NUM_DPU;
    dpu_alloc(nr_of_dpus, NULL, &dpu_set);
    printf("Allocated %d DPUs\n", nr_of_dpus);
    dpu_load(dpu_set, DPU_BINARY, NULL);

    int * data_mapping = malloc(sizeof(int) * NODE_NUM);
    set_data_mapping(data_mapping);

    distribute(dpu_set, matrix, nodes, data_mapping);

    printf("Loaded DPU binary: %s\n", DPU_BINARY);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    int i = 0;
    DPU_FOREACH(dpu_set, dpu, i) {
        DPU_ASSERT(dpu_log_read(dpu, stdout));
    }

    return 0;
}
