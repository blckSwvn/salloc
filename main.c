#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 1024);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);

// printf("allocated stuff\n");
void *a = salloc(&m, 10);
void *b = salloc(&m, 10);
void *c = salloc(&m, 10);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);


dump_a(&m);
// printf("sfreed stuff\n");
// sfree(&m, a);
sfree(&m, b);
dump_f(&m);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);
void *d = salloc(&m, 10);
dump_f(&m);
dump_a(&m);
}
