#include "sys/mman.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

typedef struct header {
	size_t length;
	char data[];
} header;

typedef struct f_header {
	header *next;
	header *prev;
} f_header;

#define BINS 16

typedef struct master {
	void *base;
	void *end;
	size_t mem_used;
	size_t mem_free;
	header *tail;
	header *freelist[BINS]; //segregated freelist, smalles size class is 16
} master;

#define SET_USED(h) ((h) | (size_t)1)
#define SET_FREE(h) ((h) & ~1)
#define GET_SIZE(h) ((h) & ~(size_t)1)
#define IS_USED(h) (((h) & 1) != 0)


#define ALIGN_TO 16
#define ALIGN(size) (((size) + ((ALIGN_TO)-1)) & ~((ALIGN_TO)-1))

static const size_t size_freelist[BINS] = {
	16, 32, 64, 128, 256, 512, 102, 2048, 4096, 8192, 16384, 32768, 65536, 131072,
	262144, 524288, 1048576, 2097152
//	1,  2,  3,  4,   5,   6,   7,    8,    9,    10,   11,    12,	 13,	14,
//	15,	16,
};

void add_to_free(master *m, header *curr);
void *split_block(master *m, void **ptr, size_t requested);

void add_to_free(master *m, header *curr){
	curr->length = SET_FREE(curr->length);
	size_t curr_size = GET_SIZE(curr->length);
	f_header *curr_f = (f_header *)((char *)curr + sizeof(header) + curr_size - sizeof(f_header));

	uint8_t i = 0;
	while(i < BINS){
		if(size_freelist[i] * 2 >= curr_size && curr_size >= size_freelist[i]){

			if(!m->freelist[i]){
				m->freelist[i] = curr;
				curr_f->next = NULL;
				curr_f->prev = NULL;
				return;
			}
			if(m->freelist[i] && m->freelist[i] != curr){
				f_header *head = (f_header *)((char *)m->freelist + sizeof(header) + GET_SIZE(m->freelist[i]->length) - sizeof(f_header));
				head->prev = curr;
				if(head->next == curr)head->next = NULL;
				curr_f->next = m->freelist[i];
				curr_f->prev = NULL;

				m->freelist[i] = curr;

				return;
			}

		}
		i++;
	}
}

void *remove_from_free(master *m, size_t requested){
	uint8_t i = 0;
	while(i < BINS){
		if(size_freelist[i] * 2 >= requested && requested >= size_freelist[i]){
			header *free = m->freelist[i];	
			while(free){
				size_t free_length = GET_SIZE(m->freelist[i]->length);
				f_header *free_f = (f_header *)((char *)free + free_length - sizeof(f_header) + sizeof(header));
				if(free_length < requested && free_f->next) free = free_f->next;

				if(free && free->length >= requested){
					if(free_f->prev){
						header *prev = free_f->prev;
						f_header *prev_f = (f_header *)((char *)prev + prev->length - sizeof(f_header) + sizeof(header));
						if(free_f->next) prev_f->next = free_f->next;
						else prev_f->next = NULL;
					}
					if(free_f->next){
						header *next = free_f->next;
						f_header *next_f = (f_header *)((char *)next + next->length - sizeof(f_header) + sizeof(header));
						if(free_f->prev) next_f->prev = free_f->prev;
						else next_f->prev = NULL;
					}
// master *m, void **ptr, size_t requested
					free = split_block(m, (void *)free->data, requested);
					SET_USED(free->length);
					return free;
				};
			}
		}
		i++;
	}
	return NULL;
}


void sfree(master *m, void *ptr) {
    if (!ptr || ptr == m->base) return;

    header *curr = (header *)((char *)ptr - offsetof(header, data));
    size_t csize = GET_SIZE(curr->length);
    size_t total = sizeof(header) + GET_SIZE(curr->length);

    m->mem_free += total;
    m->mem_used -= total;

    add_to_free(m, curr);
}

void *split_block(master *m, void **ptr, size_t requested){
	header *curr = (header *)((char *)*ptr - offsetof(header, data));
	size_t curr_length = GET_SIZE(curr->length);
	if(requested + sizeof(header) < curr_length){
		header *split = (header *)((char *)curr + sizeof(header) + GET_SIZE(curr_length) - requested - sizeof(header));
		split->length = requested - sizeof(header);
		SET_USED(split->length);
		curr->length -= requested + sizeof(header);
		sfree(m, (void *)curr->data);
		return split;
	};
	return curr;
}

bool sinit(master *m, size_t size){
	void *start = mmap(NULL, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	m->base = start;
	m->end = (char *)start + size;
	m->mem_used = 0;
	m->mem_free = size;
	m->tail = NULL;

	for(int i = 0; i < BINS; i++){
		m->freelist[i] = NULL;
	}

	return true;
}

void *salloc(master *m, size_t requested) {
	requested = ALIGN(requested);
	if(requested <= sizeof(f_header)) requested = sizeof(f_header);
	header *c = NULL;

	if(m->tail){
		c = (header*)((char*)m->tail + GET_SIZE(m->tail->length) + sizeof(header));
		c->length = SET_USED(requested);
		size_t total = sizeof(header) + GET_SIZE(c->length);
		m->tail = c;
		m->mem_free -= total;
		m->mem_used += total;
		return c->data;
	} else {
		c = (header*)m->base;
		c->length = SET_USED(requested);
		size_t total = sizeof(header) + GET_SIZE(c->length);
		m->tail = c;
		m->mem_free -= total;
		m->mem_used += total;
		return c->data;
	}
}




// void srealloc(master *m, void **ptr, size_t requested){
// 	header *c = (header *)((char *)*ptr - offsetof(header, data));
// 	header *n = (header *)((char *)c + sizeof(header) + GET_SIZE(c->length));
// 	f_header *nf = n ?(f_header *)((char *)n + sizeof(header) + GET_SIZE(n->length) - sizeof(f_header)): NULL;
// 	f_header *pf = NULL;
// 	header *p2 = NULL;
// 	f_header *p2f = NULL;
//
// 	size_t old_length = requested;
// 	if(requested <= sizeof(f_header))requested = sizeof(f_header);
// 	requested = ALIGN(requested);
//
// 	//try to merge next in adjasent memory
// 	n = (header *)((char *)c + sizeof(header) + GET_SIZE(c->length));
// 	if(n && !IS_USED(n->length) && requested + sizeof(header) <= GET_SIZE(n->length)){
// 		if(nf && nf->prev) {
// 			f_header *pf = (f_header *)((char *)nf->prev + sizeof(header) + GET_SIZE(nf->prev->length) - sizeof(f_header));
// 			if(nf->next) pf->next = nf->next;
// 			else pf->next = NULL;
// 		}
//
// 		if(nf && nf->next) {
// 			f_header *n2f = (f_header *)((char *)nf->next + sizeof(header) + GET_SIZE(nf->next->length) - sizeof(f_header));
// 			if(nf->prev) n2f->prev = nf->prev;
// 			else n2f->prev = NULL;
// 		}
// 		SET_USED(n->length);
// 		n->length += requested + sizeof(header);
// 		*ptr = (void *)n->data;
// 		f_header *oldf = (f_header *)((char *)m->freelist + sizeof(header) + GET_SIZE(m->freelist->length) - sizeof(f_header));
// 		oldf->prev = c;
// 		m->freelist = c;
// 		return; 
// 	}
//
// 	header *new = salloc(m, old_length);
// 	memcpy(new, c->data, old_length);
// 	*ptr = (void *)new->data;
// 	return;
// }

void dump_f(master *m) {
	header *c = NULL;
	uint8_t i = 0;
	while(i){
		if(m->freelist[i]) c = m->freelist[i];
		size_t nmr = 0;
		printf("dump_f\n");
		printf("m->freelist: %p freelist_size: %zu sizeof(f_header): %zu sizeof(header): %zu \n", m->freelist, size_freelist[i], sizeof(f_header), sizeof(header));
		while(c){
			f_header *cf = (f_header*)((char *)c + sizeof(header) + GET_SIZE(c->length) - sizeof(f_header));
			void *next = NULL;
			void *prev = NULL;
			if(cf->next) next = cf->next;
			if(cf->prev) prev = cf->prev;
			//Count | ptr | Free/Used | length 
			printf("%zu | node: %p | %d | length: %zu | next: %p | prev: %p\n",
					nmr, c, IS_USED(c->length), GET_SIZE(c->length), next, prev );
			nmr++;
			c = cf->next; 
		}
		i++;
		printf("\n");
	}
}

void dump_a(master *m) {
	header *c = (header *)m->base;
	size_t nmr = 0;
	void *end = ((char *)c + sizeof(header) + GET_SIZE(c->length));
	printf("dump_a\n");
	printf("m->freelist: %p\n", m->freelist);
	while(c && c->length && (char *)end < (char *)m->end){
		f_header *cf = (f_header*)((char *)c + sizeof(header) + GET_SIZE(c->length) - sizeof(f_header));
		void *next = NULL;
		void *prev = NULL;
		if(cf->next) next = cf->next;
		if(cf->prev) prev = cf->prev;
	//Count | ptr | Free/Used | length 
	printf("%zu | node: %p | %d | length: %zu | next: %p | prev: %p\n",
	nmr, c, IS_USED(c->length), GET_SIZE(c->length), next, prev );
	nmr++;
	end = ((char *)c + sizeof(header) + GET_SIZE(c->length));
	c = (header *)((char *)c + GET_SIZE(c->length) + sizeof(header));
	}
	printf("\n");
}

