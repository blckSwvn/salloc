#include <stdio.h>
#include "salloc.h"

int main(){
master m;

sinit(&m, 100);
printf("Allocator initialized\n");

printf("allocated stuff\n");
void *a = salloc(&m, 14);
dump_a(&m);
}
