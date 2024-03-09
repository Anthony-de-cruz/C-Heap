#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct ChunkData {
    uint32_t size;
    bool in_use;
    struct ChunkData *next;
} ChunkData;

typedef struct HeapData {
    uint32_t available;
    ChunkData *head;
} HeapData;

int init_heap(HeapData *heap) {

    uint32_t page_size = getpagesize();

    // System call to get a page-aligned memory map
    void *map = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (map == (void *)-1) {
        perror("mmap");
        return 1;
    }

    // Build a starting heap chunk out of the memory map
    ChunkData *head = (ChunkData *)map;
    head->size = page_size - sizeof(ChunkData);
    head->in_use = false;
    head->next = NULL;

    heap->head = head;
    heap->available = 100000; // TODO: Calculate availability

    return 0;
}

ChunkData *truncate_chunk(ChunkData *chunk, uint32_t new_size) {

    if (new_size >= chunk->size + sizeof(ChunkData)) {
        fprintf(stderr, "Invalid size for truncation: %d, chunk size: %d\n",
                new_size, chunk->size);
        return (void *)-1;
    }

    // The chunk must be truncated down to a
    // multiple of the size of the metadata
    uint32_t truncated_size =
        new_size + (sizeof(ChunkData) - (new_size % sizeof(ChunkData)));
    uint32_t leftover_space = chunk->size - truncated_size;

    chunk->size = truncated_size;

    // Create a new chunk at the end of the old chunk
    ChunkData *new_chunk =
        (ChunkData *)((char *)chunk + sizeof(ChunkData) + chunk->size);
    new_chunk->size = leftover_space - sizeof(ChunkData);
    new_chunk->in_use = false;
    new_chunk->next = chunk->next;

    chunk->next = new_chunk;

    return new_chunk;
}

void *heap_alloc(HeapData *heap, uint32_t size) {

    // Traverse till we find a free chunk that is big enough
    ChunkData *free_chunk = heap->head;
    while (free_chunk->in_use || free_chunk->size <= size) {
        if (free_chunk->next == NULL) {
            fprintf(stderr, "Inadequate amount of memory available\n");
            return (void *)-1;
        }
        free_chunk = free_chunk->next;
    }

    ChunkData *new_chunk = truncate_chunk(free_chunk, size);
    if (new_chunk == (void *)-1) {
        fprintf(stderr, "Invalid chunk truncation\n");
        return (void *)-1;
    }

    free_chunk->in_use = true;

    return free_chunk + sizeof(ChunkData);
}

void heap_free(HeapData *heap, void *ptr) {

    // TODO: Implement
}

int main(int argc, char *argv[]) {

    HeapData heap = {0};
    if (init_heap(&heap)) {
        return EXIT_FAILURE;
    }

    char *string_1 = heap_alloc(&heap, 100);
    strcat(string_1, "hi there");

    char *string_2 = heap_alloc(&heap, 150);
    strcat(string_2, "hello");

    printf("%s\n", string_1);
    printf("%s\n", string_2);

    return EXIT_SUCCESS;
}
