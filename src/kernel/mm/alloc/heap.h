/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_ALLOC_HEAP_H
#define UKO_OS_KERNEL__MM_ALLOC_HEAP_H 1

#include "segment.h"
#include <list.h>

/**
 * A collection of segments, which can allocate objects of various sizes. Each
 * hart gets its own heap.
 */
struct mm_alloc_heap {
  /**
   * The hart that owns the heap, and all the segments and pages pointed to by
   * it.
   */
  struct hart *hart;

  /**
   * Pointers to pages that hopefully contain free blocks for small objects.
   *
   * This goes in steps of 8 (the smallest size class) instead of going by
   * powers of two like `pages` (or simply not having these pointers and using
   * `pages` directly), because "next multiple of 8" is generally faster to
   * compute than "next power of two," even with modern bit manipulation
   * instructions.
   */
  struct mm_alloc_page *pages_direct[128];

  /**
   * The list heads for pages owned by this heap of the given sizes.
   *
   * - 0:         size <= 8    (small)
   * - 1:    8  < size <= 16   (small)
   * - 2:    16 < size <= 32   (small)
   * - 3:    32 < size <= 64   (small)
   * - 4:    64 < size <= 128  (small)
   * - 5:   128 < size <= 256  (small)
   * - 6:   256 < size <= 512  (small)
   * - 7:   512 < size <= 1KiB (small)
   * - 8:  1KiB < size <= 2KiB (small)
   * - 9:  2KiB < size <= 4KiB (small)
   * - 10: 4KiB < size <= 8KiB (small)
   * - 11:   8KiB < size <= 16KiB  (large)
   * - 12:  16KiB < size <= 32KiB  (large)
   * - 13:  32KiB < size <= 64KiB  (large)
   * - 14:  64KiB < size <= 128KiB (large)
   * - 15: 128KiB < size <= 256KiB (large)
   * - 16: 256KiB < size <= 512KiB (large)
   * - 17: 512KiB < size           (huge)
   */
  struct list_head pages[18];

  /**
   * The linked list of pages for small objects that are unused, but part of
   * segments that have some used pages.
   *
   */
  struct list_head unused_pages;

  /**
   * The list heads for full pages. Keeping these separate saves us from having
   * to traverse full pages whenever we go through the `pages` lists.
   *
   * Pages get moved here when allocation fails, and are moved back after a
   * local `free()` from them occurs. When a remote `free()` occurs, the freed
   * object gets pushed to the `delayed_free` list, which gets freed (as local
   * `free()`s!) just before going through this list.
   */
  struct list_head full_pages;

  /**
   * The "delayed free list." When every block is allocated in a page, it gets
   * moved to the `full_pages` list, and only a local `free()` will move it out
   * of that list. The first remote `free()` that happens gets pushed to this
   * list instead of the normal `remote_free` free list, so that they can be
   * turned into local `free()`s that happen later.
   *
   * Note that this is an ordinary linked list, not an XORred list, since we
   * don't otherwise know what pages blocks correspond to.
   */
  struct mm_alloc_block *delayed_free;
};

/**
 * Allocates a heap, but does not initialize it.
 */
struct mm_alloc_heap *heap_alloc(void);

/**
 * Initializes an already-allocated heap, using memory allocated for a 4MiB
 * segment as its initial memory.
 */
void heap_init(struct mm_alloc_heap *heap, struct mm_alloc_segment *segment);

/**
 * Updates the pages_direct entries for the heap corresponding to the given size
 * class. It is a no-op to call this on a size class that does not have
 * pages_direct entries.
 */
void heap_update_pages_direct(struct mm_alloc_heap *heap, usize size_class);

/**
 * Pushes a page into the unused pages list.
 */
void heap_push_unused_page(struct mm_alloc_heap *heap,
                           struct mm_alloc_page *page);

/**
 * Returns whether the size class is one that shows up in the pages_direct
 * array.
 */
static inline bool size_class_in_pages_direct(usize size_class) {
  return size_of_size_class(size_class) <= 1024;
}

/**
 * Given an allocation size, returns the index into the pages_direct of a heap
 * corresponding to a page for objects of a given size.
 */
static inline usize pages_direct_index_of_size(usize size) {
  assert(0 < size && size <= 1024);

  // Round up the size; the smallest size class is 8 bytes.
  size = (size + 7) & ~(usize)7;

  // Compute the index into pages_direct.
  usize pages_direct_index = (size >> 3) - 1;
  assert(pages_direct_index < 128);
  return pages_direct_index;
}

/**
 * Given an index into the pages_direct of a heap, return the largest object
 * that "belongs" to that slot.
 */
static inline usize size_of_pages_direct_index(usize pages_direct_index) {
  assert(pages_direct_index < 128);
  return (pages_direct_index + 1) << 3;
}

#endif // UKO_OS_KERNEL__MM_ALLOC_HEAP_H
