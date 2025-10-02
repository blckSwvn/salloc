#include <stdio.h>
#include <string.h>
#include "salloc.h"  // your allocator header

int main() {
    master m;

    // Initialize allocator with 1MB
    if (!sinit(&m, 1024 * 1024, 0)) {
        printf("Allocator init failed\n");
        return 1;
    }

    printf("Allocator initialized\n");
    dump_m(&m);
    dump_f(&m);

    // Allocate a few blocks
    void *a = salloc(&m, 128);
    void *b = salloc(&m, 256);
    void *c = salloc(&m, 64);

    // Write to them
    memset(a, 0xAA, 128);
    memset(b, 0xBB, 256);
    memset(c, 0xCC, 64);

    printf("\nAfter allocations:\n");
    dump_m(&m);
    dump_f(&m);
    dump_a(&m);

    // Free b
    sfree(&m, b);
    printf("\nAfter freeing b:\n");
    dump_m(&m);
    dump_f(&m);
    dump_a(&m);

    // Reallocate a to bigger size
    // a = srealloc(&m, a, 512);
    // if (a) memset(a, 0xDD, 512);

    printf("\nAfter reallocating a to 512 bytes:\n");
    dump_m(&m);
    dump_f(&m);
    dump_a(&m);

    // Free everything
    sfree(&m, a);
    sfree(&m, c);
    printf("\nAfter freeing all:\n");
    dump_m(&m);
    dump_f(&m);
    dump_a(&m);

    return 0;
}
