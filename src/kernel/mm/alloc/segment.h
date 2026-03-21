/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_ALLOC_SEGMENT_H
#define UKO_OS_KERNEL__MM_ALLOC_SEGMENT_H 1

#include "page.h"
#include "size_classes.h"

struct mm_alloc_heap;

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
   * The number of pages that are not in the `unused_pages` list. When this
   * reaches zero, the segment is freed.
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
   * The number of pages depends on the object size; segments that contain pages
   * for small objects have 64 pages, while segments that contain pages for
   * large or huge pages have 1 page.
   */
  struct mm_alloc_page pages[64];
};

static_assert(sizeof(struct mm_alloc_segment) <= (1 << 12));
static_assert(alignof(struct mm_alloc_segment) <= (1 << SEGMENT_SHIFT));

/**
 * Allocates a segment, but does not initialize it.
 */
struct mm_alloc_segment *segment_alloc(usize size);

/**
 * Initializes an already-allocated segment for pages that contain small
 * objects.
 *
 * The pages of this segment get pushed to the heap's unused pages list.
 */
void segment_init_small(struct mm_alloc_segment *segment,
                        struct mm_alloc_heap *heap);

/**
 * Initializes an already-allocated segment for a page that contains large
 * objects.
 *
 * The page of this segment is not linked into any list.
 */
void segment_init_large(struct mm_alloc_segment *segment);

/**
 * Initializes an already-allocated segment for a huge object with the given
 * size.
 *
 * The page of this segment is not linked into any list.
 */
void segment_init_huge(struct mm_alloc_segment *segment);

/**
 * Frees a segment.
 */
void segment_free(struct mm_alloc_segment *segment);

/**
 * Gets the segment corresponding to an allocation.
 */
struct mm_alloc_segment *segment_of_ptr(uptr ptr);

#endif // UKO_OS_KERNEL__MM_ALLOC_SEGMENT_H
