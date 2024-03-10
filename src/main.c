#include <stdlib.h>
#include <string.h>

#include "heap.h"

int main(int argc, char *argv[]) {
    HeapData heap = {0};
    if (heap_init(&heap)) {
        return EXIT_FAILURE;
    }

    char *string_1 = chunk_alloc(&heap, 100);
    strcat(string_1, "hi there");

    char *string_2 = chunk_alloc(&heap, 150);
    strcat(string_2, "hello");

    char *string_3 = chunk_alloc(&heap, sizeof(char) * 10);
    strcat(string_3, "testings");

    print_heap_chunks(stdout, &heap);

    chunk_free(string_2);
    chunk_free(string_3);

    print_heap_chunks(stdout, &heap);

    if (heap_deconstruct(&heap)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
