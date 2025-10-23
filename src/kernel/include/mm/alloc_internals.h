/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_ALLOC_INTERNALS_H
#define UKO_OS_KERNEL__MM_ALLOC_INTERNALS_H 1

#include <list.h>

/**
 * The internals of the allocator, which is based on the design of mimalloc.
 * This is an unstable interface.
 */

/* A quick overview of the allocator follows. Terminology is as used in the
 * technical report, _not_ as used in the current state of the mimalloc repo.
 *
 * ## Data structures
 *
 * Instances of the allocator are "heaps" (`struct mm_alloc_heap`). Allocating
 * from a heap is not thread-safe or re-entrant, but freeing is. As a result,
 * every hart has its own heap.
 */

struct mm_alloc_heap;

/* Each heap owns several "segments" (`struct mm_alloc_segment`). A segment
 * starts at a 4MiB boundary, and are mostly used to amortize calls to the
 * physical and virtual memory allocators.
 */

struct mm_alloc_segment;

/**
 * The log2 of the size of a segment that contains pages for small or large
 * objects.
 */
static constexpr usize MM_ALLOC_SEGMENT_SHIFT = 22;

/* Each segment is composed of one or more "pages." These should not be confused
 * with hardware pages; while hardware pages are typically 4KiB, allocator pages
 * are at least 64KiB. The segment stores all the metadata for pages; the first
 * page is shortened accordingly.
 */

/**
 * The metadata for a apge.
 */
struct mm_alloc_page;

/* Each page has several free lists composed of "blocks"
 * (`struct mm_alloc_block`). Blocks are just links in a linked list.
 */

/**
 * An item in a free list.
 */
struct mm_alloc_block {
  /**
   * The address of the next block, XORred by the `xor_cookie` from the page.
   */
  uaddr next;
};

/* ## Object sizes
 *
 * Objects can be "small," "large," or "huge." This is determined by the size:
 *
 * - small: `size < 8KiB`
 * - large: `8KiB <= size < 512KiB`
 * - huge: `512KiB <= size`
 *
 * Small objects get a special fast path in the allocator. The pages that
 * contain them are 64KiB, and segments that contain small-object pages contain
 * 64 pages (and so are 4MiB in size).
 */

/**
 * The number of bits to shift the difference between a heap address and the
 * address of its segment by in order to get the index of the page it belongs
 * to, when the page contains small objects.
 */
static constexpr usize MM_ALLOC_PAGE_SMALL_SHIFT = 16;

/* Pages that contain large objects are 4MiB in size, and segments that contain
 * them only contain one page (and so are 4MiB in size).
 */

/**
 * The number of bits to shift the difference between a heap address and the
 * address of its segment by in order to get the index of the page it belongs
 * to, when the page contains large objects.
 */
static constexpr usize MM_ALLOC_PAGE_LARGE_SHIFT = 22;

/* Pages that contain huge objects only contain a single object. They can be
 * larger or smaller than 4MiB, and their segments are accordingly sized.
 */

/**
 * The number of bits to shift the difference between a heap address and the
 * address of its segment by in order to get the index of the page it belongs
 * to, when the page contains huge objects.
 */
static constexpr usize MM_ALLOC_PAGE_HUGE_SHIFT = 22;

/* ## Data structure definitions
 */

/**
 * Data in a page that needs to be accessed atomically by other harts than the
 * one that owns the page.
 */
union mm_alloc_page_remote {
  uptr bits;
  struct {
    /**
     * A flag for whether the page is in the `full_pages` list, so a `free()`
     * from a remote hart should push to the heap's `delayed_free` list
     * instead.
     *
     * This will cause the page to get removed from the `full_pages` list the
     * next time the owning hart fails to find a free page with free blocks in
     * any `pages` list.
     */
    bool needs_delayed_free : 1;

    /**
     * The free list that `free()` pushes to when called from a remote hart
     * (other than when the `needs_delayed_free` flag is set).
     */
    uptr remote_free : (8 * sizeof(uptr) - 1);
  };
};

static_assert(sizeof(union mm_alloc_page_remote) == sizeof(uptr));

/**
 * The metadata associated with a page.
 */
struct mm_alloc_page {
  /**
   * Pages are stored in an intrusive doubly linked list for their size class
   * (or `unused_pages`, for empty pages).
   */
  struct list_head list;

  /**
   * The address of the head of the free list that can be allocated from, XORred
   * by the `xor_cookie` from the page.
   */
  uaddr free;

  /**
   * The address of the head of the free list that `free()` pushes to when
   * called from the hart that owns the page, XORred by the `xor_cookie` from
   * the page.
   */
  uaddr local_free;

  /**
   * The data that needs to be atomically accessed when `free()` is called from
   * a remote hart (a hart other than the one that owns the page).
   *
   * TODO: This should be XORred too.
   */
  atomic union mm_alloc_page_remote remote;

  /**
   * The "cookie" that addresses in the free list are XORred by.
   */
  uaddr xor_cookie;

  /**
   * The number of blocks that have been handed out and not returned to
   * `local_free`. This is used so that we can detect when a page becomes empty
   * in `free()`.
   */
  u32 used_blocks;

  /**
   * The length of the `remote.remote_free` list.
   */
  atomic u32 remote_free_length;

  /**
   * The size class of objects in the page. Zero if the page is unused.
   */
  u8 size_class : 7;

  /**
   * A flag for whether the page is in the `full_pages` list. The next time a
   * local `free()` happens while this flag is set, the page is moved from the
   * `full_pages` list to the appropriate one for its size class.
   */
  bool in_full_list : 1;
};

/**
 * The shared metadata for one or more pages. Allocated at a 4MiB boundary.
 */
struct mm_alloc_segment {
  /**
   * The ID of the hart that allocates from pages in the segment. When other
   * harts free memory, they push to `remote_free` instead of `local_free`.
   */
  u64 hart_id;

  /**
   * The number of pages that are not in the `unused_pages` list.
   */
  usize used_pages;

  /**
   * The number of bits to shift the difference between a heap address and the
   * address of its segment by in order to get the index of the page it belongs
   * to.
   *
   * This should be one of `MM_ALLOC_PAGE_SMALL_SHIFT`,
   * `MM_ALLOC_PAGE_LARGE_SHIFT`, or `MM_ALLOC_PAGE_HUGE_SHIFT`.
   */
  usize page_shift;

  /**
   * The metadata for pages.
   *
   * The number of pages depends on the object size.
   */
  struct mm_alloc_page pages[];
};

/**
 * A collection of segments, which can allocate objects of various sizes. Each
 * hart gets its own heap.
 */
struct mm_alloc_heap {
  /**
   * The ID of the hart that owns the heap, and all the segments and pages
   * pointed to by it.
   */
  u64 hart_id;

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
   * - 0: 8
   * - 1: 16
   * - 2: 32
   * - 3: 64
   * - 4: 128
   * - 5: 256
   * - 6: 512
   * - 7: 1KiB
   * - 8: 2KiB
   * - 9: 4KiB
   * - 10: 8KiB
   * - 11: 16KiB
   * - 12: 32KiB
   * - 13: 64KiB
   * - 14: 128KiB
   * - 15: 256KiB
   * - 16: >=512KiB
   */
  struct list_head pages[17];

  /**
   * The list heads for pages that are unused, i.e. not part of any size class.
   * These pages are all for small objects (i.e., the pages are 64KiB).
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
   */
  struct mm_alloc_block *delayed_free;
};

/**
 * Initializes the heap used by the boothart.
 */
void mm_alloc_boothart_heap_init(struct mm_alloc_heap *heap,
                                 struct mm_alloc_segment *segment);

#endif // UKO_OS_KERNEL__MM_ALLOC_INTERNALS_H
