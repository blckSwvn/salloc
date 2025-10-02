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
void sfree(master *m, void *ptr){
	if(ptr == m->base){
		internal_skill(m);
		return;
	}

	node *n = (node *)((char *)ptr - offsetof(node, data));
	node *curr = n;
	char *end = (char *)n + sizeof(node) + n->length;


	node **c_prev = (node **)(end - 2 * sizeof(node*));
	node **c_next = (node **)(end - 1 * sizeof(node*));
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
	char *end = (char *)curr + sizeof(node) + curr->length;
	if(*end) {
		node **c_next = (node **)(end - 1 * sizeof(node*));
		node **c_prev = (node **)(end - 2 * sizeof(node*));
		while(curr){
			if((c_next && c_next) && curr->length > requested) {
				node *prev = *c_prev;
				char *prev_end = (char *)prev + sizeof(node) + prev->length;
				node **prev_next = (node **)(prev_end - 2 * sizeof(node*));
				node **c_next = (node **)(curr - 2 * sizeof(node*));
				node *next = *c_next;
				char *next_end = (char *)next + sizeof(node) + next->length;
				node **next_prev = (node **)(next_end - 2 * sizeof(node*));
				*next_prev = *c_prev;
				*prev_next = *c_next;
				*c_next = NULL;
				*c_prev = NULL;
			}

		}
	}
	if(curr){
		if(curr->length >= requested * 2){
			node *splitN = (node*)((char *)curr + sizeof(node) + requested);
			splitN->length = curr->length - requested;
			curr->length = requested;
			sfree(m, splitN);
		}
		return curr->data;
	} else {
		if(m->memFree < requested+sizeof(node)){
			return NULL;
		}
		m->memFree -= requested + sizeof(node); 
		m->memUsed += requested + sizeof(node);
		node *newN = NULL;
		if(m->tail == NULL){
			newN = (node *)m->tail;
		} else {
			newN = (node*)((char *)m->tail + sizeof(node) + m->tail->length);
		}
		newN->length = requested;
		newN->dead = false;
		m->tail = newN;
		return newN->data;
	}
}

void *srealloc(master *m, void *ptr, size_t requested) {
	if(!ptr) return salloc(m, requested);  // realloc(NULL, size) behaves like malloc

	node *curr = (node *)((char *)ptr - sizeof(node));
	size_t old_size = curr->length;

	size_t total_size = old_size;
	node *next = (node *)((char *)curr + sizeof(node) + curr->length);

	while(next && (char *)next < (char *)m->end && next->dead) {
		total_size += sizeof(node) + next->length;

		if (next->prev) next->prev->next = next->next;
		if (next->next) next->next->prev = next->prev;
		if (m->freelist == next) m->freelist = next->next;
		next = (node *)((char *)next + sizeof(node) + next->length);
	}

	if(total_size >= requested) {
		if (total_size >= requested + sizeof(node) + 1) {
			node *splitN = (node *)((char *)curr + sizeof(node) + requested);
			splitN->length = total_size - requested - sizeof(node);
			splitN->dead = true;

			splitN->next = m->freelist;
			splitN->prev = NULL;
			if (m->freelist) m->freelist->prev = splitN;
			m->freelist = splitN;

			curr->length = requested;
		} else {
			curr->length = total_size;
		}
		curr->dead = false;
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
	node *curr;
	if(m && m->freelist){
		curr = m->freelist;
		while(curr){
			printf("==== free list ====\n");
			printf("node: %p | %s | size: %zu | next: %p | prev: %p\n",
					curr, curr->dead ? "FREE" : "USED", curr->length, curr->next, curr->prev);
			curr = curr->next;
		}
		printf("\n");
	} else {
		printf("m->freelist = NULL or m == NULL ");
	}
}

void dump_a(master *m){
	if(!m || !m->base) return;

	dump_m(m);

	char *ptr = (char *)m->base;
	char *end = ptr + m->memUsed;
	size_t nmr = 0;
	node *curr;

	printf("\n ==== all nodes ====\n");
	while(ptr < end) {
		curr = (node*)ptr;
		printf(" %zu | node: %p | %s | size: %zu | next: %p | prev: %p\n",
				nmr, curr, curr->dead ? "FREE" : "USED", curr->length, curr->prev, curr->next); //next and prev are swapped in dump but it behaves correctly

		ptr += sizeof(node) + curr->length;
		nmr++;
	}
	printf("\n");
}
