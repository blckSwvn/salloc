#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>

typedef struct node {
	bool dead;
	size_t length;
	struct node *next;
	struct node *prev;
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
	m->tail = NULL;
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
void sfree(void *ptr, master *m){
    if(ptr == m->base){
        internal_skill(m);
        return;
    }

    node *n = (node *)((char *)ptr - offsetof(node, data));
    node *curr = n;
    node *next = m->freelist;

    n->dead = true;
    n->next = next;
    n->prev = NULL;
    m->freelist = n;

    if(next) next->prev = n;

    next = (node *)((char *)curr + sizeof(node) + curr->length);
    while(next && (char *)next < (char *)m->end && next->dead){
        if(next->prev) next->prev->next = next->next;
        if(next->next) next->next->prev = next->prev;

        curr->length += sizeof(node) + next->length;
        next = (node *)((char *)next + sizeof(node) + next->length);
    }
}

void *salloc(master *m, size_t requested){
	node *curr = m->freelist;
	while(curr){
		if(curr->dead && curr->length > requested){
			curr->dead = false;
			break;
		}
		curr = curr->next;
	}
	if(curr){
		if(curr->length >= requested * 2){
			node *splitN = (node*)((char *)curr + sizeof(node) + requested);
			splitN->length = curr->length - requested;
			curr->length = requested;
			splitN->next = NULL;
			splitN->dead = true;
			sfree(splitN, m);
		}
		return curr->data;
	} else {
		if(m->memFree < requested+sizeof(node)){
			return NULL;
		}
		m->memFree -= sizeof(node) + requested;
		m->memUsed += requested + sizeof(node);
		node *newN;
		if(m->tail){
			newN = (node*)((char *)m->tail + sizeof(node) + m->tail->length);
		} else {
			newN = (node *)m->base;
		}
		m->tail = newN;
		newN->length = requested;
		newN->dead = false;
		newN->next = NULL;
		return newN->data;
	}
}

void *srealloc(void *ptr, master *m, size_t requested){
node *n = (node *)((char *)ptr - offsetof(node, data));
node *next = (node *)((char *)n + n->length + sizeof(node));
if ((char*)next >= (char*)m->end) next = NULL;
if (next->dead){
	n->length = n->length + next->length + sizeof(node);
	return n->data;
} else {
	if(m->memFree >= requested+sizeof(node)){
		void *newData = salloc(m, requested);
		if(newData){
			unsigned char *d = newData;
			const unsigned char *s = n->data;
			size_t len = n->length;
			while(len--){
				*d++ = *s++;
			}
			sfree(n, m);
			return newData;
		}
		return NULL;
	}
}
return NULL;
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
               nmr, curr, curr->dead ? "FREE" : "USED", curr->length, curr->next, curr->prev);

        ptr += sizeof(node) + curr->length;
        nmr++;
    }
    printf("\n");
}
