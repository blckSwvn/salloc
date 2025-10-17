#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 1024);
int size = 3;
void *b = salloc(&m, 100);
void *ptr = salloc(&m, size * sizeof(int));
void *d = salloc(&m, sizeof(long));
void *c = salloc(&m, sizeof(int));
void *idk = salloc(&m, 10);
printf("m->mem_used: %zu\n", m.mem_used);
printf("m->mem_free: %zu\n", m.mem_free);

dump_a(&m);
sfree(&m, d);
dump_a(&m);
sfree(&m, b);
dump_a (&m);
// sfree(&m, ptr);
// dump_a (&m);
sfree(&m, idk);
sfree(&m, c);
dump_a(&m);
sfree(&m, ptr);
dump_a(&m);
}
