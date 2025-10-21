/**
 * xfer_file_to_dpu.c
 * Stream a host binary file into a single DPU's MRAM heap at a byte offset.
 *
 * Usage:
 *   DPU_FOREACH(dpu_set, dpu) {
 *       int rc = xfer_file_to_dpu(dpu, "Task0.bin", some_offset);
 *       assert(rc == 0);
 *   }
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dpu.h>
#include <assert.h>

#ifndef DPU_MRAM_HEAP_POINTER_NAME
#define DPU_MRAM_HEAP_POINTER_NAME "__mram_heap_base"
#endif

// Return 0 on success, -1 on error (prints a message to stderr on error).
int xfer_file_to_dpu(struct dpu_set_t dpu, const char *filename, size_t dst_offset) {
    int ret = -1;
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "xfer_file_to_dpu: cannot open '%s': %s\n", filename, strerror(errno));
        return -1;
    }

    // Determine file size
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "xfer_file_to_dpu: fseek(SEEK_END) failed for '%s'\n", filename);
        goto out_close;
    }
    long fsz_long = ftell(fp);
    if (fsz_long < 0) {
        fprintf(stderr, "xfer_file_to_dpu: ftell failed for '%s'\n", filename);
        goto out_close;
    }
    size_t fsz = (size_t)fsz_long;
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fprintf(stderr, "xfer_file_to_dpu: fseek(SEEK_SET) failed for '%s'\n", filename);
        goto out_close;
    }

    // Read file into memory
    void *buf = malloc(fsz ? fsz : 1);
    if (!buf) {
        fprintf(stderr, "xfer_file_to_dpu: OOM allocating %zu bytes\n", fsz);
        goto out_close;
    }
    size_t nread = fread(buf, 1, fsz, fp);
    if (nread != fsz) {
        fprintf(stderr, "xfer_file_to_dpu: short read on '%s' (%zu/%zu)\n", filename, nread, fsz);
        goto out_free;
    }

    // Stage pointer for this DPU then push the transfer (matches your convention)
    DPU_ASSERT(dpu_prepare_xfer(dpu, buf));
    DPU_ASSERT(dpu_push_xfer(
        dpu,                       // single DPU set
        DPU_XFER_TO_DPU,
        DPU_MRAM_HEAP_POINTER_NAME,
        dst_offset,
        fsz,
        DPU_XFER_DEFAULT));

    ret = 0; // success

out_free:
    free(buf);
out_close:
    fclose(fp);
    return ret;
}
// ================= Configuration =================
#define INPUT_ROOT "input_bins"
#define NUM_EXECUTIONS 5
#define NUM_CORES 50
#define DPU_BINARY "./bin/GEN_device"
#define MRAM_OFFSET 0
// =================================================

int main(void) {
    struct dpu_set_t dpu_set, dpu;

    // Allocate DPUs
    DPU_ASSERT(dpu_alloc(NUM_CORES, NULL, &dpu_set));
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));

    for (unsigned exec_id = 0; exec_id < NUM_EXECUTIONS; ++exec_id) {
        printf("\n=== Execution %u ===\n", exec_id);

        unsigned core_id = 0;
        DPU_FOREACH(dpu_set, dpu) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/execution_%u/core_%u.bin",
                     INPUT_ROOT, exec_id, core_id);

            printf("Loading %s into DPU %u...\n", filepath, core_id);
            int rc = xfer_file_to_dpu(dpu, filepath, MRAM_OFFSET);
            if (rc != 0) {
                fprintf(stderr, "Failed to transfer %s\n", filepath);
                return 1;
            }

            core_id++;
            if (core_id >= NUM_CORES)
                break;
        }

        printf("Launching all DPUs for execution %u...\n", exec_id);
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        DPU_FOREACH(dpu_set, dpu) {
            DPU_ASSERT(dpu_log_read(dpu, stdout));
        }

        printf("Execution %u complete.\n", exec_id);
    }

    dpu_free(dpu_set);
    printf("\nAll executions completed.\n");
    return 0;
}