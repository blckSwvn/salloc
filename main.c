#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 1024);
void *a = salloc(&m, 10);
void *d = salloc(&m, 100);
void *b = salloc(&m, 10);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);

// printf("allocated stuff\n");
// printf("m->mem_used: %zu\n", m.mem_used);
// printf("m->mem_free: %zu\n", m.mem_free);

// printf("sfreed stuff\n");
sfree(&m, d);
dump_f(&m);
dump_a(&m);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);
void *e = salloc(&m, 100);
sfree(&m, a);
dump_f(&m);
dump_a(&m);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);
}
