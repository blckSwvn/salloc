#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 1000, true);
printf("Allocator initialized\n");
dump_m(&m);

printf("allocated stuff\n");
void *a = salloc(&m, 4);
void *b = salloc(&m, 4);
void *c = salloc(&m, 4);
void *d = salloc(&m, 40);
dump_f(&m);
dump_a(&m);

printf("freed stuff\n");
sfree(&m, a);
sfree(&m, b);
sfree(&m, c);
sfree(&m, d);
dump_f(&m);
dump_a(&m);
}
