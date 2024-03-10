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
    struct ChunkData *prev;
} ChunkData;

typedef struct HeapData {
    uint32_t available;
    ChunkData *head;
} HeapData;

void print_void_data(FILE *restrict stream, void *ptr) {
    ChunkData *chunk = (ChunkData *)(char *)ptr - sizeof(ChunkData);
    fprintf(stream,
            "chunk @ %p\n    size: %iu\n    in use: %d\n    next: %p\n    "
            "prev: %p\n",
            chunk, chunk->size, chunk->in_use, chunk->next, chunk->prev);
}

void print_chunk_data(FILE *restrict stream, ChunkData *chunk) {
    fprintf(stream,
            "chunk @ %p\n    size: %iu\n    in use: %d\n    next: %p\n    "
            "prev: %p\n",
            chunk, chunk->size, chunk->in_use, chunk->next, chunk->prev);
}

void print_heap_chunks(FILE *restrict stream, HeapData *heap) {
    ChunkData *chunk = heap->head;
    do {
        print_chunk_data(stdout, chunk);
        chunk = chunk->next;
    } while (chunk != heap->head);
}

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
    head->next = head;
    head->prev = head;

    heap->head = head;
    heap->available = head->size; // TODO: Calculate availability

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
    new_chunk->prev = chunk;

    chunk->next = new_chunk;

    if (chunk->prev == chunk) {
        chunk->prev = new_chunk;
    }

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
        print_chunk_data(stderr, new_chunk);
        return (void *)-1;
    }

    free_chunk->in_use = true;

    fprintf(stdout, "alloc @ %p\n", free_chunk);
    return free_chunk + sizeof(ChunkData);
}

void coalesce_chunk(ChunkData *chunk) {
    // Scan the prev chunk and coalesce if not in use
    if (!chunk->prev->in_use) {
        chunk->prev->size += chunk->size + sizeof(ChunkData);
        chunk->prev->next = chunk->next;
        chunk = chunk->prev;
    }
    // Scan the next chunk and coalesce if not in use
    if (!chunk->next->in_use) {
        chunk->size += chunk->next->size + sizeof(ChunkData);
        chunk->next = chunk->next->next;
    }
}

void chunk_free(void *ptr) {
    ChunkData *chunk = (ChunkData *)(char *)ptr - sizeof(ChunkData);
    chunk->in_use = false;

    fprintf(stdout, "free @ %p\n", chunk);

    coalesce_chunk(chunk);
}
