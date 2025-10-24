#include <stddef.h>
#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 1024);
void *a = salloc(&m, 10 * sizeof(int));
void *a2 = salloc(&m, 10 * sizeof(int));
void *a3 = salloc(&m, 10 * sizeof(int));
void *a4 = salloc(&m, 10 * sizeof(int));
void *a5 = salloc(&m, 40 * sizeof(int));
void *b = salloc(&m, 20 * sizeof(int));
// dump_a(&m);
sfree(&m, a);
sfree(&m, a2);
// dump_a(&m);
// void *new = salloc(&m, 1 * sizeof(int));
sfree(&m, a3);
sfree(&m, a4);
sfree(&m, a5);
sfree(&m, b);
dump_a(&m);
dump_f(&m);
}
