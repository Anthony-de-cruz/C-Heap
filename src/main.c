#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
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

int heap_init(HeapData *heap) {
    uint32_t page_size = getpagesize();

    // System call to get a page-aligned memory map
    void *map = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (map == (void *)-1) {
        perror("mmap");
        return -1;
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

int heap_deconstruct(HeapData *heap) {
    // System call to free the mapped memory
    if (munmap((void *)heap->head, getpagesize())) {
        perror("munmap");
        return -1;
    }

    return 0;
}

ChunkData *chunk_truncate(ChunkData *chunk, uint32_t new_size) {
    ulong data_size = sizeof(ChunkData);

    if (new_size >= chunk->size + data_size) {
        fprintf(stderr, "Invalid size for truncation: %d, chunk size: %d\n",
                new_size, chunk->size);
        return (void *)-1;
    }

    // The chunk must be truncated down to a
    // multiple of the size of the metadata
    uint32_t truncated_size = new_size + (data_size - (new_size % data_size));
    uint32_t leftover_space = chunk->size - truncated_size;

    chunk->size = truncated_size;

    // Create a new chunk at the end of the old chunk
    ChunkData *new_chunk =
        (ChunkData *)((char *)chunk + data_size + chunk->size);
    new_chunk->size = leftover_space - data_size;
    new_chunk->in_use = false;
    new_chunk->next = chunk->next;

    chunk->next = new_chunk;

    return new_chunk;
}

void *chunk_alloc(HeapData *heap, uint32_t size) {
    // Traverse till we find a free chunk that is big enough
    ChunkData *free_chunk = heap->head;
    while (free_chunk->in_use || free_chunk->size <= size) {
        if (free_chunk->next == NULL) {
            fprintf(stderr, "Inadequate amount of memory available\n");
            return (void *)-1;
        }
        free_chunk = free_chunk->next;
    }

    ChunkData *new_chunk = chunk_truncate(free_chunk, size);
    if (new_chunk == (void *)-1) {
        fprintf(stderr, "Invalid chunk truncation\n");
        return (void *)-1;
    }

    free_chunk->in_use = true;

    return free_chunk + sizeof(ChunkData);
}

void coalesce_chunk(HeapData *heap, ChunkData *chunk) {

    // TODO: Implement
}

void chunk_free(HeapData *heap, void *ptr) {
    ChunkData *chunk = (ChunkData *)(char *)ptr - sizeof(ChunkData);
    chunk->in_use = false;

    coalesce_chunk(heap, chunk);
}

void print_chunk_data(FILE *restrict stream, void *ptr) {
    ChunkData *chunk = (ChunkData *)(char *)ptr - sizeof(ChunkData);
    fprintf(stream, "chunk @ %p\n    size: %iu\n    in use: %d\n    next: %p\n",
            chunk, chunk->size, chunk->in_use, chunk->next);
}

int main(int argc, char *argv[]) {
    HeapData heap = {0};
    if (heap_init(&heap)) {
        return EXIT_FAILURE;
    }

    char *string_1 = chunk_alloc(&heap, 100);
    strcat(string_1, "hi there");
    print_chunk_data(stdout, string_1);

    char *string_2 = chunk_alloc(&heap, 150);
    strcat(string_2, "hello");
    print_chunk_data(stdout, string_2);

    char *string_3 = chunk_alloc(&heap, sizeof(char) * 10);
    strcat(string_3, "testings");
    print_chunk_data(stdout, string_3);

    printf("%s\n", string_1);
    printf("%s\n", string_2);
    printf("%s\n", string_3);

    chunk_free(&heap, string_2);

    print_chunk_data(stdout, string_2);

    if (heap_deconstruct(&heap)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
