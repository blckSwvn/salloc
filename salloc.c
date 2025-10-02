#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>

typedef struct node {
	size_t length;
	char data[];
} node;

typedef struct master {
	void *base;
	size_t memUsed;
	size_t memFree;
	node *freelist;
	node *tail;
	void *end;
	bool is_mmap;
} master;

#define END(n) ((char *)(n) + sizeof(node) + (n)->length)
#define PREV_P(n) (node **)(END(n) - 2 * sizeof(node*))
#define NEXT_P(n) (node **)(END(n) - 1 * sizeof(node*))

#define PREV(n) (*PREV_P(n))
#define NEXT(n) (*NEXT_P(n))


static void internal_sinit(master *m, void *start, void *end, bool is_mmap) {
	m->base = start;
	m->end = end;
	m->memUsed = 0;
	m->memFree = (size_t)((char*)end - (char*)start);
	m->freelist = NULL;
	m->tail = m->base;
	m->is_mmap = is_mmap;
}

bool sinit(master *m, size_t size, bool use_mmap) {
	void *start = NULL;
	if (use_mmap) {
		start = mmap(NULL, size, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (start == MAP_FAILED) return false;
	} else {
		start = sbrk(size);
		if (start == (void*) -1) return false;
	}

	internal_sinit(m, start, (char*)start + size, use_mmap);
	return true;
}

void internal_skill(master *m) {
	size_t size = (char*)m->end - (char*)m->base;
	if (m->is_mmap) {
		munmap(m->base, size);
	} else {
		brk(m->base);
	}

}

// typedef struct node {
// 	size_t length;
// 	char data[];
// } node;

void sfree(master *m, void *ptr){
	if(ptr == m->base){
		internal_skill(m);
		return;
	}

	node *n = (node *)((char *)ptr - offsetof(node, data));
	node *curr = n;

	
	node **c_prev = PREV_P(curr);
	node **c_next = NEXT_P(curr);
	if(m->freelist) {
		*c_next = m->freelist;
		*c_prev = NULL;
		if(m->freelist) { // make sure freelist head is valid
			char *freelist_end = (char*)m->freelist + sizeof(node) + m->freelist->length;
			node **freelist_prev = (node **)(freelist_end - 2 * sizeof(node*));
			if(freelist_prev) *freelist_prev = n;
		}
	} else {
		*c_next = NULL;
		*c_prev = NULL;
	}
	m->memFree += curr->length + sizeof(node);
	m->memUsed -= curr->length + sizeof(node);
	m->freelist = n;
	m->tail = curr;

	//self-explanatory
	while((char *)curr < (char *)m->end) {
		char *end = (char *)curr + sizeof(node) + curr->length;
		node **c_prev = (node **)(end - 2 * sizeof(node*));
		node **c_next = (node **)(end - 1 * sizeof(node*));
		if(!*c_prev || !*c_next) return;
		node *prev = *c_prev;
		char *prev_end = (char *)prev + sizeof(node) + prev->length;
		node **prev_prev = (node **)(prev_end - 2 * sizeof(node*));
		node **prev_next = (node **)(prev_end - 1 * sizeof(node*));
		if(prev_next && prev_prev){
			*prev_next = *c_next;
			node *next = *c_next;
			if(!next) return;
			char *next_end = (char *)next + sizeof(node) + next->length;
			node **next_prev = (node **)(next_end - 2 * sizeof(node*));
			if(*next_prev) *next_prev = prev;
			prev->length += curr->length + sizeof(node);
			curr = next;
		} else {
			return;
		}
	}
}

void *salloc(master *m, size_t requested){
	node *curr = m->freelist;
	if(requested < 2 * sizeof(node*)) requested += 2 * sizeof(node*);
	while(curr){
		// char *end = END(curr);
		node **c_prev = NEXT_P(curr);
		node **c_next = PREV_P(curr);
		if (curr->length >= requested){
			node *next = NEXT(curr);
			// char *next_end = END(next);
			node **next_prev = PREV_P(next);
			*next_prev = *c_prev;
			node *prev = *c_prev;
			// char *prev_end = END(prev);
			node **prev_next = NEXT_P(prev);
			*prev_next = *c_next;
			return curr->data;
		}
		if(curr->length >= requested * 2 ) {
			node *splitN = (node *)((char *)curr + sizeof(node) + requested);
			splitN->length = curr->length - requested;
			curr->length = requested;
			sfree(m, splitN);
			return curr->data;
		}
		curr = *c_next;
	}
	if(m->memFree < requested + sizeof(node)) return NULL;
	else {
		m->memFree -= requested + sizeof(node);
		m->memUsed += requested + sizeof(node);
	} 
	node *newN = NULL;
	if(m->tail) {
		newN = (node *)((char *)m->tail + sizeof(node) + m->tail->length);
		newN->length = requested;
		m->tail = newN;
		// node *tail = m->tail;
		// char *tail_end = END(tail);
		// node **tail_next = NEXT_P(tail);
		return newN->data;
	} 
	else {
		newN = (node *)((char *)m + sizeof(node));
		newN->length = requested;
		m->tail = newN;
		return newN->data;
	} 
}

void *srealloc(master *m, void *ptr, size_t requested) {
	if(!ptr) return salloc(m, requested);  // realloc(NULL, size) behaves like malloc

	node *curr = (node *)((char *)ptr - sizeof(node));
	size_t old_size = curr->length;

	size_t total_size = old_size;
	node *next = NEXT(curr);
	node **next_next = NEXT_P(next);
	node **next_prev = PREV_P(next);

	while(next && (char *)next < (char *)m->end && (*next_next && *next_prev)) {
		total_size += sizeof(node) + next->length;
		node **curr_next = NEXT_P(curr);
		// node **curr_prev = PREV_P(curr);
		node *next2 = NEXT(next);
		node **next2_prev = PREV_P(next2);

		if(next_prev) *curr_next = *next_next;
		if(next_next) *next2_prev = *next_prev;	
		if(m->freelist == next) m->freelist = next2;
		next = (node *)((char *)next + sizeof(node) + next->length);
	}

		if (total_size >= requested + sizeof(node) + 1 + 2 * sizeof(node*)) {
			node *splitN = (node *)((char *)curr + sizeof(node) + requested);
			splitN->length = total_size - requested - sizeof(node);
			sfree(m, splitN);


			curr->length = requested;
			if(!m->freelist) curr->length = total_size;
		return curr->data;
	}

	void *newData = salloc(m, requested);
	if (!newData) return NULL;

	size_t toCopy = old_size < requested ? old_size : requested;
	memcpy(newData, curr->data, toCopy);

	sfree(m, curr->data);
	return newData;
}

void dump_m(master *m){
	if(m){
		printf("==== master ====\n");
		printf("master: %p | base: %p | end: %p | used: %zu | free: %zu | freelist: %p | tail: %p\n",
				m, m->base, m->end, m->memUsed, m->memFree, m->freelist, m->tail);
	} else {
		printf("m = NULL");
	}
}

void dump_f(master *m){
	if(!m && !m->freelist){
		printf("m->freelist = NULL or m == NULL ");
		return;
	}	
	node *curr = m->freelist;
	while(curr){
		node **curr_next = NEXT_P(curr);
		node **curr_prev = PREV_P(curr);
		printf("==== free list ====\n");
		printf("node: %p | FREE | size: %zu | next: %p | prev: %p\n",
				curr, curr->length, *curr_next, *curr_prev);
		curr = *curr_next;
	}
	printf("\n");
}

// helper: check if a node is in freelist
static bool in_freelist(master *m, node *n) {
    node *curr = m->freelist;
    while (curr) {
        if (curr == n) return true;
        node **next = NEXT_P(curr);
        curr = *next;
    }
    return false;
}

void dump_a(master *m) {
    if (!m || !m->base) return;

    dump_m(m);

    char *ptr = (char *)m->base;
    size_t nmr = 0;
    node *curr = (node *)ptr;

    printf("\n ==== all nodes ====\n");

    while (curr && (char *)curr < (char *)m->tail) {
        bool is_free = in_freelist(m, curr);

        printf(" %zu | node: %p | %s | size: %zu\n",
               nmr, curr,
               is_free ? "FREE" : "USED",
               curr->length);

        curr = (node *)((char *)curr + sizeof(node) + curr->length);
        nmr++;
    }
    printf("\n");
}
