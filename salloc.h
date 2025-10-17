#ifndef SALLOC_H
#define SALLOC_H

#include <stddef.h>
#include <stdbool.h>

typedef struct header header;

typedef struct master {
	void *base;
	void *end;
	size_t mem_used;
	size_t mem_free;
	header *tail;
	header *freelist;
} master;

bool sinit(master *m, size_t requested);
void *salloc(master *m, size_t requested);
void sfree(master *m, void *ptr);
void *srealloc(master *m, void *ptr, size_t requested);
void dump_a(master *m);
void dump_f(master *m);

#endif
