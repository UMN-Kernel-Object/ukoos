# Overview

ukoOS has multiple strategies for memory management, that manage memory at different levels.

- The kernel has a standard memory allocator, accessed with the functions `alloc` and `free`.
  These functions act similarly to `malloc` and `free` in ordinary C.

  The allocator that handles these requests is called **the heap memory allocator**.

- The kernel keeps track of all of RAM, and hands out pages to be mapped into userspace processes and to be used by the heap memory allocator.

  This allocator is currently not capable of allocating more than a single contiguous page, but could be extended to support this in the future.
  This allocator is called **the physical memory allocator**.

- The kernel manages its own virtual memory, in the RAM region of the memory map.

  This is a buddy allocator, and can serve power-of-two sized allocation requests.
  The allocator that handles these requests is called **the virtual memory allocator**.

Each hart has its own root page table, since it can be running a different userspace process.
However, the kernel's memory map is kept in sync between all harts.

Higher-half memory is only rarely mapped and unmapped, so relatively inefficient mechanisms (a full TLB shootdown) can be used to ensure all harts have the same view of it.
