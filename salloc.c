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


#define ALIGN_TO 16
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
	if(requested <= sizeof(f_header)) requested = sizeof(f_header);

	header *c = m->freelist;
	while(c){
		f_header *cf = (f_header *)((char *)c + sizeof(header) + GET_SIZE(c->length) - sizeof(f_header));
		if(requested <= GET_SIZE(c->length) + sizeof(header)) {
			if(cf->next) {
				f_header *nf = (f_header *)((char *)cf->next + GET_SIZE(cf->next->length) - sizeof(f_header));
				nf->prev = cf->prev ? nf->prev = cf->prev: NULL;
			}
			if(cf->prev) {
				f_header *pf = (f_header *)((char *)cf->prev + GET_SIZE(cf->prev->length) - sizeof(f_header));
				pf->next = cf->prev ? pf->next = cf->next: NULL;
			} 
			if(GET_SIZE(c->length) > requested + sizeof(header) && GET_SIZE(c->length) - requested - sizeof(header) >= sizeof(f_header) + sizeof(header)) {
				c->length -= requested + sizeof(header);
				header *split = (header *)((char *)c + sizeof(header) + GET_SIZE(c->length));
				cf = (f_header *)((char *)c + sizeof(header) + GET_SIZE(c->length) - sizeof(f_header));
				split->length = SET_USED(requested);
				cf->next = m->freelist && m->freelist != c ? m->freelist: NULL;
				cf->prev = NULL;
				m->freelist = c;
				f_header *sf = (f_header *)((char *)split + sizeof(header) + GET_SIZE(split->length) - sizeof(f_header));
				sf->next = NULL;
				sf->prev = NULL;
				return split->data;
			}
			m->freelist = cf->next ? cf->next: NULL;
			cf->next = NULL;
			cf->prev = NULL;
			c->length = SET_USED(c->length);
			return c->data;
		}
		if(cf->next) c = cf->next;
		else break;
	}
	if(requested + sizeof(header) > m->mem_free) return NULL;

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


void sfree(master *m, void *ptr) {
    if (!ptr || ptr == m->base) return;

    header *c = (header *)((char *)ptr - offsetof(header, data));
    c->length = SET_FREE(c->length);
    size_t csize = GET_SIZE(c->length);
    size_t total = sizeof(header) + GET_SIZE(c->length);
    f_header *cf = (f_header *)((char *)c + sizeof(header) + csize - sizeof(f_header));

    m->mem_free += total;
    m->mem_used -= total;

    if(!m->freelist){
	    m->freelist = c;
	    cf->next = NULL;
	    cf->prev = NULL;
	    return;
    }
    if(m->freelist && m->freelist != c){
	    f_header *mf = (f_header *)((char *)m->freelist + sizeof(header) + GET_SIZE(m->freelist->length) - sizeof(f_header));
	    if(mf->next == c){
		    //insert into freelist
		    cf->next = m->freelist;
		    cf->prev = NULL;
		    mf->prev = c;

		    m->freelist = c;

		    return;
		    }
	    }
    header *old = m->freelist;
    f_header *oldf = (f_header *)((char *)old + sizeof(header) + GET_SIZE(old->length) - sizeof(f_header));

    cf->next = old;
    cf->prev = NULL;
    oldf->prev = c;
    m->freelist = c;


    f_header *pf = NULL;
    header *p = NULL;
    header *p2 = NULL;
    header *n = NULL;
    f_header *nf = NULL;

    //check if there is a previous block in memory
    if ((char *)c > (char *)m->base + sizeof(header)) {
        //previous block's footer is just before current header
        pf = (f_header *)((char *)c - sizeof(header) + sizeof(f_header));
        if (pf && pf->prev) {
            p2 = pf->prev;                   // previous free block header
            f_header *p2f = (f_header *)((char *)p2 + sizeof(header) + GET_SIZE(p2->length) - sizeof(f_header));
	    p = p2f->next;
	    if(cf->next) p2f->next = cf->next;
	    else p2f->next = NULL;
	    n = (header *)((char *)c + sizeof(header) + csize);
	    if(n && !IS_USED(n->length)){
		    nf = (f_header *)((char *)n + sizeof(header) + GET_SIZE(n->length) - sizeof(f_header));
		    if(pf->prev) nf->prev = pf->prev;
		    else nf->prev = NULL;
	    }
            if (!IS_USED(p->length)) {
                // merge previous block into current
                p->length += sizeof(header) + csize;
                // c = p;
                cf = (f_header *)((char *)c + sizeof(header) + GET_SIZE(c->length) - sizeof(f_header));
            }
        }
    }

    n = (header *)((char *)c + sizeof(header) + GET_SIZE(c->length));
    if ((char *)n < (char *)m->end && !IS_USED(n->length)) {
        size_t nsize = GET_SIZE(n->length);
        f_header *nf = (f_header *)((char *)n + sizeof(header) + nsize - sizeof(f_header));

        // unlink next block from freelist
        if (nf->prev) {
            f_header *prev_nf = (f_header *)((char *)nf->prev + sizeof(header) + GET_SIZE(nf->prev->length) - sizeof(f_header));
            prev_nf->next = nf->next;
        } else {
            m->freelist = nf->next;
        }
        if (nf->next) {
            f_header *next_nf = (f_header *)((char *)nf->next + sizeof(header) + GET_SIZE(nf->next->length) - sizeof(f_header));
            next_nf->prev = nf->prev;
        }

        // merge current block with next
        c->length += sizeof(header) + nsize;
        cf = (f_header *)((char *)c + sizeof(header) + GET_SIZE(c->length) - sizeof(f_header));
    }
}

void srealloc(master *m, void **ptr, size_t requested){
	header *c = (header *)((char *)*ptr - offsetof(header, data));
	header *n = (header *)((char *)c + sizeof(header) + GET_SIZE(c->length));
	f_header *nf = n ?(f_header *)((char *)n + sizeof(header) + GET_SIZE(n->length) - sizeof(f_header)): NULL;
	f_header *pf = NULL;
	header *p2 = NULL;
	f_header *p2f = NULL;
	
	size_t old_length = requested;
	if(requested <= sizeof(f_header))requested = sizeof(f_header);
	requested = ALIGN(requested);

	//try to merge next in adjasent memory
	n = (header *)((char *)c + sizeof(header) + GET_SIZE(c->length));
	if(n && !IS_USED(n->length) && requested + sizeof(header) <= GET_SIZE(n->length)){
		if(nf && nf->prev) {
			f_header *pf = (f_header *)((char *)nf->prev + sizeof(header) + GET_SIZE(nf->prev->length) - sizeof(f_header));
			if(nf->next) pf->next = nf->next;
			else pf->next = NULL;
		}

		if(nf && nf->next) {
			f_header *n2f = (f_header *)((char *)nf->next + sizeof(header) + GET_SIZE(nf->next->length) - sizeof(f_header));
			if(nf->prev) n2f->prev = nf->prev;
			else n2f->prev = NULL;
		}
		SET_USED(n->length);
		n->length += requested + sizeof(header);
		*ptr = (void *)n->data;
		f_header *oldf = (f_header *)((char *)m->freelist + sizeof(header) + GET_SIZE(m->freelist->length) - sizeof(f_header));
		oldf->prev = c;
		m->freelist = c;
		return; 
	}

	header *new = salloc(m, old_length);
	memcpy(new, c->data, old_length);
	*ptr = (void *)new->data;
	return;
}

void dump_f(master *m) {
	header *c = NULL;
	if(m->freelist) c = m->freelist;
	size_t nmr = 0;
	printf("dump_f\n");
	printf("m->freelist: %p sizeof(f_header): %zu sizeof(header): %zu \n", m->freelist, sizeof(f_header), sizeof(header));
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

