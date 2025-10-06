#include "sys/mman.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

typedef struct header {
	size_t length;
	char data[];
} header;

typedef struct f_header {
	header *next;
	header *prev;
} f_header;

typedef struct master {
	void *base;
	void *end;
	size_t mem_used;
	size_t mem_free;
	header *tail;
	header *freelist;
} master;

#define SET_USED(h) ((h) | (size_t)1)
#define SET_FREE(h) ((h) & ~1)
#define GET_SIZE(h) ((h) & ~(size_t)1)
#define IS_USED(h) (((h) & 1) != 0)

#define ALIGN_TO 8
#define ALIGN(size) (((size) + ((ALIGN_TO)-1)) & ~((ALIGN_TO)-1))

bool sinit(master *m, size_t size){
	void *start = mmap(NULL, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	m->base = start;
	m->end = (char *)start + size;
	m->mem_used = 0;
	m->mem_free = size;
	m->tail = NULL;
	m->freelist = NULL;

	return true;
}

void *salloc(master *m, size_t requested) {
	requested = ALIGN(requested);
	if(requested <= sizeof(f_header))requested = sizeof(f_header);

	header *c = m->freelist;
	while(c){
		f_header *cf = (f_header*)((char *)c + sizeof(header) + GET_SIZE(c->length) - sizeof(f_header));
		header *n;
		f_header *nf;
		header *p;
		f_header *pf;
		if(requested <= GET_SIZE(c->length) + sizeof(header)) {
			if(cf->next) {
				n = cf->next;
				nf = (f_header*)((char *)n + sizeof(header) + GET_SIZE(n->length) - sizeof(f_header));
			}
			if(cf->prev) {
				if(nf->prev) nf->prev = cf->prev;
				if(nf->prev) p = cf->prev;
				pf = (f_header*)((char*)p + sizeof(header) + GET_SIZE(p->length) - sizeof(f_header));
			} 
			if(pf->next) {
				if(n) pf->next = n;
			}
			c->length = SET_USED(c->length);
			return c->data;
		}
		if(cf->next) c = cf->next;
	}
	if(requested + sizeof(header) > m->mem_free) return NULL;

	if(m->tail){
		c = (header*)((char*)m->tail + GET_SIZE(m->tail->length) + sizeof(header));
		c->length = SET_USED(requested);
		m->tail = c;
		m->mem_free -= GET_SIZE(c->length) - sizeof(header);
		m->mem_used += GET_SIZE(c->length) + sizeof(header);
		return c->data;
	} else {
		c = (header*)((char *)m->base + sizeof(header));
		c->length = SET_USED(requested);
		m->tail = c;
		m->mem_free -= GET_SIZE(c->length) - sizeof(header);
		m->mem_used += GET_SIZE(c->length) + sizeof(header);
		return c->data;
	}
}

void sfree(master *m, void *ptr){
	if(ptr == m->base){
		printf("kill arena\n");
		return;
	}

	header *c = (header *)((char *)ptr - offsetof(header, data));
	c->length = SET_FREE(c->length);
	f_header *cf = (f_header *)((char *)c + sizeof(header) + GET_SIZE(c->length) - sizeof(f_header));
	cf->next = NULL;
	cf->prev = NULL;
	if(m->freelist){
		cf->next = m->freelist;
		f_header *mf = (f_header *)((char *)m->freelist + sizeof(header) + GET_SIZE(m->freelist->length) - sizeof(f_header));
		if(mf->prev) mf->prev = c;
		m->freelist = c;
	} else {
		m->freelist = c;
	}

	header *p = NULL;
	if((char *)c > (char *)m->base) {
		p = (header *)((char*)c - sizeof(header) - GET_SIZE(c->length));
	}

	f_header *pf = NULL;
	if((char *)c > (char *)m->base){
		if(p && !IS_USED(p->length)){ //merges backwards in adjasent memory
			f_header *pf = (f_header *)((char *)p + sizeof(header) + GET_SIZE(p->length) - sizeof(f_header));
			if((char *)p > (char *)m->base && !IS_USED(p->length)) {
				if(cf->prev){
					printf("cf->prev: %p", cf->prev);
					f_header *p2f = (f_header *)((char *)p + sizeof(header) + GET_SIZE(p->length) - sizeof(f_header));
					if(p2f->next) p2f->next = p;
				}
			}
			header *n = (header *)((char *)c + sizeof(header) + GET_SIZE(c->length));
			if(n && !IS_USED(n->length)){
				f_header *nf = (f_header *)((char *)n + sizeof(header) + GET_SIZE(n->length) - sizeof(f_header));
				if(pf->prev && nf->next) nf->prev = c;
			}

			//merges p to extend to n in length
			p->length += GET_SIZE(c->length) + sizeof(header);
			printf("merged prev lengths\n");
		}
		header *n = (header *)((char *)c + GET_SIZE(c->length) + sizeof(header)); //merges forward in adjasent memory
		if(n && !IS_USED(n->length)){
			f_header *nf = (f_header *)((char *)n + sizeof(header) + GET_SIZE(n->length) - sizeof(f_header));
			if(cf->prev) nf->prev = cf->prev;
			if(nf->next){
				f_header *n2f = (f_header *)((char *)nf->next + sizeof(header) + GET_SIZE(nf->next->length) - sizeof(f_header));
				if(n2f->prev) n2f->prev = c;
			}
			//merges c to extend to n in length
			printf("merged next length \n");
			c->length += GET_SIZE(n->length) + sizeof(header);
		}
	}
}



void srealloc(master *m, header *n){

}

void dump_f(master *m) {
	header *c = NULL;
	if(m->freelist) c = m->freelist;
	size_t nmr = 0;
	printf("dump_f\n");
	printf("m->freelist: %p\n", m->freelist);
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
	printf("\n");
}

void dump_a(master *m) {
	header *c = (header *)((char *)m->base + sizeof(header));
	size_t nmr = 0;
	void *end = ((char *)c + sizeof(header) + GET_SIZE(c->length));
	printf("dump_a\n");
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
