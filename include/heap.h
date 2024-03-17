#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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

void print_void_data(FILE *stream, void *ptr);

void print_chunk_data(FILE *stream, ChunkData *chunk);

void print_heap_chunks(FILE *stream, HeapData *heap);

int heap_init(HeapData *heap);

int heap_deconstruct(HeapData *heap);

void *chunk_alloc(HeapData *heap, uint32_t size);

void chunk_free(void *ptr);

#endif // ! HEAP_H
