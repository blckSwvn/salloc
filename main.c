#include <stdio.h>
#include "salloc.h"

int main() {
    master m;
    if(!sinit(&m, 1024, false)) {
        printf("Pool init failed\n");
        return 1;
    }

    printf("=== Pool after init ===\n");
    dump_a(&m);

    int *arr[10];
    for(int i=0;i<10;i++){
        arr[i] = salloc(&m, sizeof(int));
        if(arr[i]) *arr[i] = i * 10;
    }

    printf("=== Pool after allocations ===\n");
    dump_a(&m);

    for(int i=0;i<10;i++){
        if(i%2==0 && arr[i]) sfree(arr[i], &m); // free every second node
    }

    printf("=== Pool after frees ===\n");
    dump_a(&m);

    void *temp = salloc(&m, 25);
    dump_a(&m);
    sfree (temp, &m);
    dump_a(&m);
    void *temp2 = salloc(&m, sizeof(int));
    dump_a(&m);

    return 0;
}
