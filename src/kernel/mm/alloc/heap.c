/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "heap.h"
#include <hart_locals.h>

void heap_init(struct mm_alloc_heap *heap, struct mm_alloc_segment *segment) {
  // Initialize the heap. pages_direct is full of null pointers, though,
  // which is normally illegal -- we fix that below.
  *heap = (struct mm_alloc_heap){
      .hart_id = get_hart_locals()->hart_id,
      .pages_direct = {0}, // initialized below
      .pages = {0},        // initialized below
      .unused_pages = LIST_INIT(heap->unused_pages),
      .full_pages = LIST_INIT(heap->full_pages),
      .delayed_free = nullptr,
  };
  for (usize i = 0; i < ARRAY_SIZE(heap->pages); i++)
    heap->pages[i] = LIST_INIT(heap->pages[i]);

  // Initialize the segment.
  segment_init_small(segment, heap);

  // Allocate pages of the right size classes to initialize the pages_direct
  // array.
  for (usize size_class = 0; size_class < 8; size_class++)
    assert(page_new_small(heap, size_class));

  // Check that we did actually initialize the entire pages_direct array.
  for (usize i = 0; i < ARRAY_SIZE(heap->pages_direct); i++) {
    assert(heap->pages_direct[i]);
    assert(size_class_of_size(size_of_pages_direct_index(i)) ==
           heap->pages_direct[i]->size_class);
  }
}

void heap_update_pages_direct(struct mm_alloc_heap *heap, usize size_class) {
  assert(!list_is_empty(&heap->pages[size_class]));
  struct mm_alloc_page *page =
      container_of(heap->pages[size_class].next, struct mm_alloc_page, list);

  if (!size_class_in_pages_direct(size_class))
    return;

  // Get the size of the smallest object of this size class.
  usize smallest_size;
  if (size_class == 0)
    smallest_size = 1;
  else
    smallest_size = size_of_size_class(size_class - 1) + 1;

  // Find the index of the smallest and largest object of this size class.
  usize smallest_index = pages_direct_index_of_size(smallest_size);
  usize largest_index =
      pages_direct_index_of_size(size_of_size_class(size_class));

  // Set each entry.
  for (usize i = smallest_index; i <= largest_index; i++) {
    assert(size_class_of_size(size_of_pages_direct_index(i)) == size_class);
    heap->pages_direct[i] = page;
  }
}

void heap_push_unused_page(struct mm_alloc_heap *heap,
                           struct mm_alloc_page *page) {
  // Check that the page is for small objects and is not in use.
  assert(page_segment(page)->page_shift == PAGE_SMALL_SHIFT);
  assert(list_is_empty(&page->list));
  assert(page_is_empty(page));

  // Re-initialize the page.
  bzero(page, sizeof(struct mm_alloc_page));
  page->list = LIST_INIT(page->list);

  // Push it into the heap.
  list_push(&heap->unused_pages, &page->list);
}
