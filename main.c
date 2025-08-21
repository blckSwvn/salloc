#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

typedef struct node {
	bool dead;
	size_t length;
	struct node *next;
	char data[];
} node;

typedef struct master {
	void *base;
	size_t memUsed;
	size_t memFree;
	node *freelist;
	node *tail;
} master;

void poolInit(master *m, size_t requested){
	m->base = sbrk(requested);
	m->memFree = requested - sizeof(master);
	m->memUsed = 0;
	m->freelist = NULL;
	m->tail = NULL;
}

void poolKill(master *m){
	brk(m->base = 0);
}

void free(node *n, master *m){
	node *curr = n;
	node *next = (node *)((char *)curr + curr->length + sizeof(node));
	n->dead = true;
	n->next = m->freelist;
	m->freelist = n;
	while((char *)next < (char *)m->base + m->memUsed && next->dead == true){
		next->next = NULL;
		curr->length += next->length + sizeof(node);
		next = (node *)((char *)next + next->length + sizeof(node));
	}
}

void *alloc(master *m, size_t requested){
	node *curr = m->freelist;
	while(curr){
		if(curr->dead && curr->length > requested){
			curr->dead = false;
			break;
		}
		curr = curr->next;
	}
	if(curr){
		if(curr->length >= requested * 1.5){
			node *splitN = (struct node*)((char *)m->tail + sizeof(node) + m->tail->length);
			splitN->length = curr->length - requested;
			curr->length = requested;
			splitN->next = NULL;
			splitN->dead = true;
			poolFree(splitN, m);
		}
		return curr->data;
	} else {
		if(m->memFree < requested+sizeof(node)){
			return NULL;
		}
		m->memFree = m->memFree - sizeof(node) + requested;
		m->memUsed = m->memUsed + requested + sizeof(node);
		node *newN = (struct node*)((char *)m->tail + sizeof(node) + m->tail->length);
		m->tail = newN;
		newN->length = requested;
		newN->dead = false;
		newN->next = NULL;
		return newN->data;
	}
}

void *Realloc(node *n, master *m, size_t requested){
node *next = (node *)((char *)n + n->length + sizeof(node));
if (next->dead){
	n->length = n->length + next->length + sizeof(node);
	return n->data;
} else {
	if(m->memFree >= requested+sizeof(node)){
		node *newN = alloc(m, requested);
		if(newN){
			memcpy(newN->data, n->data, n->length);
			poolFree(n, m);
			return newN->data;
		}
		return NULL;
	}
}
return NULL;
}

int main(){
	master m;
}
