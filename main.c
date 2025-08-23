#include <stdio.h>
#include "salloc.h"

int main() {
    master m;
    sinit(&m, 1000, true);
    void *ptr = salloc(&m, 11);
    void *ptr2 = salloc(&m, 12);
    void *ptr3 = salloc(&m, 13);
    void *ptr4 = salloc(&m, 14);
    void *ptr5 = salloc(&m, 15);
    dump_a(&m);
    sfree(ptr3, &m);
    sfree(ptr4, &m);
    dump_a(&m);
    srealloc(ptr2, &m, 100);
    dump_a(&m);
    return 0;
}
