#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct HeapChunk {
    uint32_t size;
    bool in_use;
    struct HeapChunk *next;
} HeapChunk;

typedef struct Heap {
    uint32_t available;
    HeapChunk *head;
} Heap;

int init_heap(Heap *heap) {

    uint32_t page_size = getpagesize();

    // System call to get a page-aligned memory map
    void *map = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (map == (void *)-1) {
        perror("mmap");
        return 1;
    }

    // Build a starting heap chunk out of the memory map
    HeapChunk *head = (HeapChunk *)map;
    head->size = page_size - sizeof(HeapChunk);
    head->in_use = false;
    head->next = NULL;

    heap->head = head;

    return 0;
}

void *heap_alloc(Heap *heap, uint32_t size) {

    // Traverse till we find a free chunk
    HeapChunk chunk = *heap->head;
    while (chunk.in_use) {
        if (chunk.next == NULL) {
            fprintf(stderr, "aint it chief");
            return (void *)-1;
        }
        chunk = *chunk.next;
    }

    if (chunk.size < size) {
        fprintf(stderr, "too big");
        return (void *)-1;
    }

    // Todo: resize the chunk so that we hand them the actual amount they want
}

void heap_free(Heap *heap, void *ptr) {}

int main(int argc, char *argv[]) {

    Heap heap = {0};
    if (init_heap(&heap)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
