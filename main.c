#include <stddef.h>
#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 1024);
void *a = salloc(&m, 10 * sizeof(int));
dump_a(&m);
dump_f(&m);
sfree(&m,a);
void *b = salloc(&m, sizeof(int));
void *c = salloc(&m, 10 * sizeof(int));
dump_a(&m);
dump_f(&m);
}
