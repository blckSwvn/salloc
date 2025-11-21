#include <sys/mman.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define ALIGN_TO 16

typedef struct __attribute__((aligned(ALIGN_TO))) header {
	size_t length;
	char data[];
} header;

//footer is stored in the char data[] after its been sfreed, also aligned to 16 to make all math 16 aligned
typedef struct __attribute__((aligned(ALIGN_TO))) footer {
	header *head;
	header *next;
	header *prev;
} footer;

#define BINS 16

typedef struct master {
	void *base;
	void *end;
	size_t used;
	size_t free;
	header *tail;
	header *freelist[BINS];
} master;

#define USED_BIT        ((size_t)1)  // block is allocated
#define PREV_INUSE_BIT  ((size_t)2)  // previous block is in use
#define FLAGS_MASK      (USED_BIT | PREV_INUSE_BIT)

// Size only (without flags)
#define GET_SIZE(h)       ((h) & ~(FLAGS_MASK))

// Used flag
#define IS_USED(h)        (((h) & USED_BIT) != 0)
#define SET_USED(h)       ((h) | USED_BIT)
#define SET_FREE(h)     ((h) & ~USED_BIT)

// Prev-in-use flag
#define PREV_INUSE(h)     (((h) & PREV_INUSE_BIT) != 0)
#define SET_PREV_INUSE(h) ((h) | PREV_INUSE_BIT)
#define CLEAR_PREV_INUSE(h) ((h) & ~PREV_INUSE_BIT)

#define ALIGN(size) (((size) + ((ALIGN_TO)-1)) & ~((ALIGN_TO)-1))

static const size_t size_freelist[BINS] = {
	32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072,
	262144, 524288, 1048576
		//	1,  2,  3,   4,   5,   6,    7,    8,    9,    10,    11,    12,    13,
		//	14,     15,	16,
};


static inline footer *get_footer(header *curr){
	return (footer *)((char *)curr + sizeof(header) + GET_SIZE(curr->length) - sizeof(footer));
}


static inline uint8_t get_list(size_t size){
	uint8_t i = 0;
	while(i < BINS){
		size_t list_size = size_freelist[i];
		if(list_size * 2 >= size && size >= list_size){
			return i;
		}
		i++;
	}
	return BINS - 1;
}


static void add_to_free(master *m, header *curr, footer *curr_f){
	curr_f->head = curr;
	uint8_t i = get_list(curr->length);
	if(m->freelist[i]){
		footer *old_head_f = get_footer(m->freelist[i]);
		old_head_f->prev = curr;
		curr_f->next = m->freelist[i];
	} else curr_f->next = NULL;

	m->freelist[i] = curr;
	curr_f->prev = NULL;
}


static void remove_from_free(master *m, header *curr, footer *curr_f){
	uint8_t i = get_list(curr->length);
	header *prev = curr_f->prev;
	header *next = curr_f->next;

	if(prev){
		footer *prev_f = get_footer(prev);
		prev_f->next = next;
	} else {
		m->freelist[i] = next;
	}

	if(next){
		footer *next_f = get_footer(next);
		next_f->prev = prev;
	}
}

void sfree(master *m, void *ptr){
	header *curr = (header *)((char *)ptr - offsetof(header, data));
	size_t curr_size = GET_SIZE(curr->length);
	footer *curr_f = get_footer(curr);
	size_t total = sizeof(header) + curr_size;

	m->free += total;
	m->used -= total;

	curr->length = SET_FREE(curr->length);
	add_to_free(m, curr, curr_f);

	//performance sake to check null than if(m->tail = curr) again
	header *next = NULL;

	//merge forward
	if(m->tail != curr){
		next = (header *)((char *)curr + sizeof(header) + GET_SIZE(curr->length));
		if(m->tail == next) m->tail = curr;

		if(!IS_USED(next->length)){
			footer *next_f = get_footer(next);
			remove_from_free(m, next, next_f);
			remove_from_free(m, curr, curr_f);

			curr->length += sizeof(header) + GET_SIZE(next->length);
			curr->length = SET_PREV_INUSE(curr->length);

			footer *new_f = get_footer(curr);

			add_to_free(m, curr, new_f);
		}
	} 

	//merges backwards
	if(!PREV_INUSE(curr->length)){
		footer *prev_f = (footer *)((char *)curr - sizeof(footer));
		header *prev = prev_f->head;

		if(next) m->tail = prev;

		if(IS_USED(prev->length)) return;

		remove_from_free(m, prev, prev_f);
		remove_from_free(m, curr, curr_f);

		prev->length += total;
		prev->length = SET_PREV_INUSE(prev->length);

		footer *new_f = get_footer(prev);
		add_to_free(m, prev, new_f);
	}
}


bool sinit(master *m, size_t size){
	void *start = mmap(NULL, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	m->base = start;
	m->end = (char *)start + size;
	m->used = 0;
	m->free = size;
	m->tail = NULL;

	uint8_t i = 0;
	while(i < BINS){
		m->freelist[i] = NULL;
		i++;
	}

	return true;
}


void skill(master *m){
	munmap(m->base, (char *)m->end - (char *)m->base);
	m->base = NULL;
	m->end = NULL;
	m->used = (size_t)NULL;
	m->free = (size_t)NULL;
	m->tail = NULL;

	uint8_t i = 0;
	while(i < BINS){
		m->freelist[i] = NULL;
		i++;
	}
}


static inline void *split(master m, header *curr, footer *curr_f, size_t requested){
	size_t new_len = GET_SIZE(curr->length) - (sizeof(header) + requested);
	printf("new_len: %zu\n", new_len);
	if(sizeof(footer) >= new_len) return curr;

	printf("curr: %p\n", curr);

	remove_from_free(&m, curr, curr_f);

	// size_t new_len = GET_SIZE(curr->length) - (sizeof(header) + requested);
	new_len = ALIGN(new_len);
	header *new = (header *)((char *)curr + new_len);
	new->length = SET_USED(requested);
	new->length = CLEAR_PREV_INUSE(new->length);

	curr->length = SET_USED(new_len);
	curr->length = SET_PREV_INUSE(curr->length);
	footer *new_curr_f = get_footer(curr);
	printf("new_curr_f: %p\n", new_curr_f);
	printf("new: %p", new);

	add_to_free(&m, curr, curr_f);
	return new->data;
}


void *salloc(master *m, size_t requested){
	if(requested <= sizeof(footer)) requested = sizeof(footer);
	requested = ALIGN(requested);

	if(m->used == 0){
		uint8_t i = 0;
		while(i < BINS){
			m->freelist[i] = NULL;
			i++;
		}
		m->tail = NULL;
	}

	uint8_t i = get_list(requested);

	header *curr = m->freelist[i];
	footer *curr_f = NULL;
	while(curr){
		curr_f = get_footer(curr);
		if(curr->length > requested) break;
		curr = curr_f->next;
	}

	if(curr){
		header *new = split(*m, curr, curr_f, requested);
		curr = new;
		if (!new && curr->length >= requested){
			curr_f = get_footer(curr);
			remove_from_free(m, curr, curr_f);
			curr->length = SET_USED(curr->length);
			return curr->data;

		}
		m->free -= GET_SIZE(curr->length) + sizeof(header);
		m->used += GET_SIZE(curr->length) + sizeof(header);
		return curr->data;
	} else {
		printf("!curr: %p\n", curr);
	}


	if(m->tail){
		curr = (header *)((char *)m->tail + sizeof(header) + GET_SIZE(m->tail->length));
		m->tail = curr;
	} else {
		curr = m->base;
		m->tail = curr;
	}

	curr->length = SET_PREV_INUSE(SET_USED(requested));
	m->used += sizeof(header) + GET_SIZE(curr->length);
	m->free -= sizeof(header) + GET_SIZE(curr->length);
	return curr->data;
}


void dump_f(master *m){
	uint8_t i = 0;
	printf("dump_f\n");
	printf("sizeof(footer): %zu, sizeof(header): %zu\n", sizeof(footer), sizeof(header));
	while(i < BINS){
		header *c = NULL;
		if(m->freelist[i]) c = m->freelist[i];
		size_t nmr = 0;
		printf("m->freelist: %p freelist_size: %zu\n", m->freelist, size_freelist[i]);
		while(c && nmr < 100){
			footer *cf = (footer*)((char *)c + sizeof(header) + GET_SIZE(c->length) - sizeof(footer));
			void *next = NULL;
			void *prev = NULL;
			void *header = NULL;
			if(cf->next) next = cf->next;
			if(cf->prev) prev = cf->prev;
			if(cf->head) header = cf->head;
			//Count | ptr | Free/Used | length 
			printf("%zu | node: %p | %d, %d | length: %zu | next: %p | prev: %p | header: %p\n",
					nmr, c, IS_USED(c->length), PREV_INUSE(c->length), GET_SIZE(c->length), next, prev, header );
			nmr++;
			c = cf->next; 
		}
		i++;
		printf("\n");
	}
}


void dump_a(master *m){
	header *c = (header *)m->base;
	size_t nmr = 0;
	void *end = ((char *)c + sizeof(header) + GET_SIZE(c->length));
	printf("dump_a\n");
	printf("m free: %zu, used: %zu, base: %p, tail: %p\n", m->free, m->used, m->base, m->tail);
	while(c && c->length && (char *)end < (char *)m->end && nmr < 100){
		footer *cf = get_footer(c);

		void *next = NULL;
		void *prev = NULL;
		void *head = NULL;
		if(cf->next) next = cf->next;
		if(cf->prev) prev = cf->prev;
		if(cf->head) head = cf->head;
		//Count | ptr | Free/Used, prev_free | length 
		printf("%zu | node: %p | %d, %d | length: %zu | next: %p | prev: %p | header: %p\n",
				nmr, c, IS_USED(c->length), PREV_INUSE(c->length), GET_SIZE(c->length), next, prev, head );
		nmr++;
		end = ((char *)c + sizeof(header) + GET_SIZE(c->length));
		c = (header *)((char *)c + GET_SIZE(c->length) + sizeof(header));
	}
	printf("\n");
}
