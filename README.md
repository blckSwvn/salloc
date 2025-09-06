# salloc — simple allocator in C

`salloc` is a **custom memory allocator** I built from scratch in C.  
It’s not a drop-in replacement for `malloc`/`free`, but a simplified more hands on allocator that manages a fixed memory arenas with its own metadata and free list.

warning its a 86x 64x allocator only currently

---

## Design Choices

-**thread safety without locking** each master manages its own arena. sfree, salloc, srealloc all require a ptr to a master node so aslong each thread
has its own master there is no global state so no locking required.
- **Fixed memory arenas**: Allocations come from a pre-allocated buffer (`sinit(size)`), so the allocator is self-contained and doesn’t call `malloc` internally.  
- **Free list**: Freed blocks are linked together, making it possible to reuse them efficiently.  
- **Coalescing**: When adjacent blocks are freed, they are merged to reduce fragmentation.  
- **Non-drop-in**: Unlike `malloc`, `sfree` requires the `master` pool reference. This makes the allocator explicit and gives more controll, but not API-compatible with libc.  
- **Debugging hooks**: Functions like `dump_a` and `dump_m` provide visibility into the allocator’s state.

#TODO
- ** make sfree merge nodes backwards
- ** switch data structures for the freelist to avoid linear search for example making it segregated 
---

## Example

```c
master m;
sinit(&m, 1024, false); // Create a pool of 1024 bytes

void *a = salloc(&m, 64);
void *b = salloc(&m, 128);

sfree(&m, a);           // Free one block
b = srealloc(&m, b, 256,);

dump_a(&m);             // Print all nodes and block states
