# salloc — simple allocator in C

`salloc` is a **custom memory allocator** I built from scratch in C.  
It’s not a drop-in replacement for `malloc`/`free`, but a simplified more hands on allocator that manages a fixed memory arenas with its own metadata and free list.

warning this is purely an expermentail allocator its not meant for prod atleast yet.

---

## Design Choices

- **Thread safety without locking** since salloc sfree srealloc all work within the context of a master node so memory isnt shared or is global
- **Fixed memory arenas**: Allocations come from a pre-allocated buffer (`sinit(size)`), so the allocator is self-contained and doesn’t call `malloc` internally.  
- **Metadata per block**: Each allocation stores a small header (`node`) with block size and state (used/free).  
- **Free list**: Freed blocks are linked together, making it possible to reuse them efficiently.  
- **Coalescing**: When adjacent blocks are freed, they are merged to reduce fragmentation.  
- **Non-drop-in**: Unlike `malloc`, `sfree` requires the `master` pool reference. This makes the allocator explicit and gives more controll, but not API-compatible with libc.  
- **Debugging hooks**: Functions like `dump_a` and `dump_m` provide visibility into the allocator’s state.

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
