#ifndef UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H
#define UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H 1

#include <types.h>

/**
 * Adds a chunk to the physical allocator. Enough memory has to be given to the
 * physical allocator with this function before calling `mm_init_physical` to
 * allocate the buddy bitmaps for physical and virtual memory. This should be
 * about 64MiB at most.
 */
void mm_init_add_physical_chunk(paddr start, paddr end);

/**
 * Sets up the physical memory allocator.
 */
void mm_init_physical(paddr devicetree_start, paddr devicetree_end,
                      paddr kernel_start, paddr kernel_end, uptr *free_va_start,
                      uptr *free_va_end);

#endif // UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H
