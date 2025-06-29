#ifndef UKO_OS_KERNEL__MM_VIRTUAL_ALLOC_H
#define UKO_OS_KERNEL__MM_VIRTUAL_ALLOC_H 1

#include <types.h>

/**
 * Flags about a link of the free list.
 */
enum virtual_buddy_free_list_flags : u16 {
  /**
   * This link was allocated in static memory as part of bootstrapping the
   * allocator, and should not be freed.
   */
  VIRTUAL_BUDDY_FREE_LIST_BOOTSTRAP = 1 << 0,
};

/**
 * A link in the free list for the buddy allocator for virtual address space.
 * These are heap-allocated.
 */
struct virtual_buddy_free_list {
  /**
   * The next link in the list.
   */
  struct virtual_buddy_free_list *next;

  /**
   * Flags about this link.
   */
  enum virtual_buddy_free_list_flags flags : 12;

  /**
   * The high bits of the address of the allocatable block of memory.
   */
  uptr addr_hi_bits : (sizeof(uptr) * 8) - 12;
};

static_assert(sizeof(struct virtual_buddy_free_list) == 2 * sizeof(uptr));
static_assert(alignof(struct virtual_buddy_free_list) == alignof(uptr));

/**
 * A buddy allocator for a 26-bit space. If pages are the leaves, this covers
 * 256GiB, i.e. either the lower or upper half of Sv39 (and still a convenient
 * size on x86_64, or on ARMv8-A with a 4KiB granule).
 */
struct virtual_buddy {
  /**
   * The range of virtual addresses this allocator covers.
   */
  uaddr start, end;

  /**
   * The free lists.
   */
  struct virtual_buddy_free_list *free_lists[27];

  /**
   * The bitmap.
   */
  u8 bitmap[1 << 25];
};

/**
 * The kernel's virtual memory allocator.
 */
extern struct virtual_buddy *const mm_kernel_virtual_buddy;

/**
 * Sets up `mm_kernel_virtual_buddy`.
 */
void mm_init_virtual(uptr free_va_start, uptr free_va_end);

/**
 * Allocates a range of virtual address space from the given allocator.
 *
 * `size` must not be zero.
 */
__attribute__((nonnull(1))) uptr mm_va_alloc(struct virtual_buddy *allocator,
                                             usize size);

/**
 * Frees a range of virtual address space allocated with `mm_va_alloc`.
 *
 * `size` must be the size that was originally allocated.
 */
__attribute__((nonnull(1))) void mm_va_free(struct virtual_buddy *allocator,
                                            uptr ptr, usize size);

#endif // UKO_OS_KERNEL__MM_VIRTUAL_ALLOC_H
