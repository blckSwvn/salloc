#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 1024);
printf("Allocator initialized\n");
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);
printf("m->end: %p\n", m.end);
printf("m->base: %p\n", m.base);

printf("allocated stuff\n");
void *a = salloc(&m, 10);
void *b = salloc(&m, 10);
void *c = salloc(&m, 10);
void *d = salloc(&m, 10);
void *e = salloc(&m, 10);


dump_a(&m);
printf("sfreed stuff\n");
sfree(&m, b);
sfree(&m, c);
dump_a(&m);
dump_f(&m);
}
