#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 1024);
void *a = salloc(&m, 10);
void *b = salloc(&m, 10);
void *c = salloc(&m, 30);
void *d = salloc(&m, 60);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);

// printf("allocated stuff\n");
// printf("m->mem_used: %zu\n", m.mem_used);
// printf("m->mem_free: %zu\n", m.mem_free);

// printf("sfreed stuff\n");
sfree(&m, a);
sfree(&m, b);
sfree(&m, d);
// sfree(&m, c);
dump_f(&m);
dump_a(&m);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);
srealloc(&m, &c, 50);
printf("%p\n",((char *)c - sizeof(size_t)));
dump_a(&m);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);
// printf("m->mem_used: %zu\n", m.mem_used);
// printf("m->mem_free: %zu\n", m.mem_free);
// dump_f(&m);
// dump_a(&m);
}
