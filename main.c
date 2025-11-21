#include <stddef.h>
#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 1024);
dump_a(&m);
void *a = salloc(&m, 1);
void *a2 = salloc(&m, 9 * sizeof(int));
void *a3 = salloc(&m, 9 * sizeof(int));
void *a4 = salloc(&m, 9 * sizeof(int));
void *a5 = salloc(&m, 9 * sizeof(int));
void *b = salloc(&m, 100 * sizeof(int));
dump_a(&m);
// sfree(&m, a);
sfree(&m, b);
sfree(&m, a4);
sfree(&m, a2);
sfree(&m, a5);
sfree(&m, a3);
sfree(&m, a);
// dump_a(&m);
dump_a(&m);
void *c = salloc(&m, 1 * sizeof(int));
void *c2 = salloc(&m, 1 * sizeof(int));
// void *c3 = salloc(&m, 1 * sizeof(int));
// sfree(&m, c2);
dump_a(&m);
dump_f(&m);
skill(&m);
}
