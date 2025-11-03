#ifndef SALLOC_H
#define SALLOC_H

#include <stddef.h>
#include <stdbool.h>

struct header;

#define BINS 16

typedef struct master {
	void *base;
	void *end;
	size_t used;
	size_t free;
	struct header *tail;
	struct header *freelist[BINS]; //segregated freelist, smalles size class is 16
} master;

bool sinit(master *m, size_t requested);
void *salloc(master *m, size_t requested);
void sfree(master *m, void **ptr);
void skill(master *m);
// void *srealloc(master *m, void *ptr, size_t requested);
void dump_a(master *m);
void dump_f(master *m);

#endif
