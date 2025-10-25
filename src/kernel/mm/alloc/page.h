/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_ALLOC_PAGE_H
#define UKO_OS_KERNEL__MM_ALLOC_PAGE_H 1

#include "block.h"
#include <list.h>

struct mm_alloc_heap;
struct mm_alloc_segment;

/**
 * Data in a page that needs to be accessed atomically by other harts than the
 * one that owns the page.
 */
union mm_alloc_page_remote {
  uaddr bits;
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
     *
     * This is a `struct mm_alloc_block_ref`.
     */
    uaddr remote_free : (8 * sizeof(uptr) - 1);
  };
};

static_assert(sizeof(union mm_alloc_page_remote) == sizeof(uptr));
static_assert(alignof(union mm_alloc_page_remote) == alignof(uptr));

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
  struct mm_alloc_block_ref free;

  /**
   * The address of the head of the free list that `free()` pushes to when
   * called from the hart that owns the page, XORred by the `xor_cookie` from
   * the page.
   */
  struct mm_alloc_block_ref local_free;

  /**
   * The data that needs to be atomically accessed when `free()` is called from
   * a remote hart (a hart other than the one that owns the page).
   */
  atomic union mm_alloc_page_remote remote;

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
   * The "cookie" that addresses in the free list are XORred by.
   */
  u64 xor_cookie : 56;

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
 * Gets a new page for small objects of the given size class.
 */
struct mm_alloc_page *page_new_small(struct mm_alloc_heap *heap,
                                     usize size_class);

/**
 * Gets a new page for large objects of the given size class.
 */
struct mm_alloc_page *page_new_large(struct mm_alloc_heap *heap,
                                     usize size_class);

/**
 * Gets a new page for a huge object with the given size.
 */
struct mm_alloc_page *page_new_huge(struct mm_alloc_heap *heap, usize size);

/**
 * Marks a page as free. This might free the segment containing the page if all
 * pages are freed.
 */
void page_free(struct mm_alloc_page *page, struct mm_alloc_heap *heap);

/**
 * Gets the page corresponding to an allocation.
 */
struct mm_alloc_page *page_of_ptr(uptr ptr);

/**
 * Computes the bounds of the allocations for a page.
 */
void page_bounds(const struct mm_alloc_page *page, uptr *out_start,
                 uptr *out_end);

/**
 * Moves the local and remote free lists into the allocation free list.
 */
void page_collect(struct mm_alloc_page *page);

/**
 * Attempts to allocate from a page. Returns `nullptr` on failure.
 */
static inline void *page_free_pop(struct mm_alloc_page *page) {
  assert(page);

  // Get the address of the next block, or notice that we've OOMed.
  assert(block_ref_is_valid(page->free));
  struct mm_alloc_block *block = block_ref_deref(page, page->free);
  if (!block)
    return nullptr;

  // Pop the block from the list and return it.
  assert(block_ref_is_valid(block->next));
  page->free = block->next;
  return block;
}

/**
 * Pushes a block to the free list.
 */
void page_free_push(struct mm_alloc_page *page, struct mm_alloc_block *block,
                    usize block_size);

/**
 * Returns the index of the page in its segment.
 */
usize page_index(const struct mm_alloc_page *page);

/**
 * Returns whether an address is in-bounds for a page.
 */
bool page_in_bounds(const struct mm_alloc_page *page, uaddr addr);

/**
 * Returns whether all objects in the page have been freed.
 */
bool page_is_empty(const struct mm_alloc_page *page);

/**
 * Returns whether all objects in the page have been allocated.
 */
bool page_is_full(const struct mm_alloc_page *page);

/**
 * Pushes a block to the local free list.
 */
void page_local_free_push(struct mm_alloc_page *page,
                          struct mm_alloc_block *block);

/**
 * Returns the segment that corresponds to a page.
 */
struct mm_alloc_segment *page_segment(struct mm_alloc_page *page);

#endif // UKO_OS_KERNEL__MM_ALLOC_PAGE_H
