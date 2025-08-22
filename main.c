#include <stdio.h>
#include "salloc.h"  // your allocator header

typedef struct list {
    int value;
    struct list *next;
} list;

int main() {
    master m;
    size_t pool_size = 1024;

    // Initialize a pool (using sbrk for example)
    if (!sinit(&m, pool_size, false)) {
        printf("Failed to initialize memory pool\n");
        return 1;
    }

    printf("=== Pool after init ===\n");
    dump_a(&m);

    // Allocate 10 list nodes
    list *head = NULL;
    list *prev = NULL;
    for (int i = 0; i < 10; i++) {
        list *node = salloc(&m, sizeof(list));
        if (!node) {
            printf("Allocation failed at index %d\n", i);
            break;
        }
        node->value = i;
        node->next = NULL;

        if (!head) head = node;
        if (prev) prev->next = node;
        prev = node;
    }

    printf("\n=== Pool after allocations ===\n");
    dump_a(&m);

    // Free every other node
    list *curr = head;
    int idx = 0;
    while (curr) {
        if (idx % 2 == 0) sfree(curr, &m);
        curr = curr->next;
        idx++;
    }

    printf("\n=== Pool after freeing every other node ===\n");
    dump_a(&m);

    // Reallocate first node to bigger size
    if (head) {
        void *new_ptr = srealloc((void *)head, &m, sizeof(list) * 2);
        if (new_ptr) {
            printf("\nReallocated first node to double size\n");
        }
    }

    printf("\n=== Pool after realloc ===\n");
    dump_a(&m);

    return 0;
}
