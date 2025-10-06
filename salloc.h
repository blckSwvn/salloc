#ifndef SALLOC_H
#define SALLOC_H

#include <stddef.h>
#include <stdbool.h>

typedef struct master master;
typedef struct header header;

struct master {
	void *base;
	void *end;
	size_t mem_used;
	size_t mem_free;
	header *tail;
	header *freelist;
};

bool sinit(master *m, size_t requested);
void *salloc(master *m, size_t requested);
void sfree(master *m, void *ptr);
void dump_a(master *m);
void dump_f(master *m);

#endif
