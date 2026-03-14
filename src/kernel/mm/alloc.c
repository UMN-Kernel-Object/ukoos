/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "alloc/heap.h"
#include <align.h>
#include <mm/alloc.h>
#include <panic.h>
#include <stdatomic.h>
#include "hartlock.h"

void free(void *ptr) {
  if (!ptr)
    return;

  struct mm_alloc_segment *segment = segment_of_ptr((uptr)ptr);
  struct mm_alloc_page *page = page_of_ptr((uptr)ptr);
  struct mm_alloc_block *block = ptr;
  
  WITH_HARTLOCK(hartlock);

  if (segment->hart == get_hart_locals(hartlock)->hart) {
    // This is a local free; i.e., we're running on the hart that owns the page.
    struct mm_alloc_heap *heap = get_hart_locals(hartlock)->heap;

    page_local_free_push(page, block);
    if (page_is_empty(page)) {
      page_free(page, heap);
    } else if (page->in_full_list) {
      // TODO: Does something have to be done with delayed freeing?
      list_remove(&page->list);
      list_push(&heap->pages[page->size_class], &page->list);

      // The page is no longer full, so it doesn't need a delayed free. Clear
      // the .needs_delayed_free flag.
      atomic uaddr *remote = (atomic uaddr *)&page->remote;
      atomic_fetch_and(remote, ~1);

      page->in_full_list = false;
    }
  } else {
    // This is a remote free.
    TODO("remote free");
  }
}

void *alloc_small(usize size, struct hartlock *hartlock, struct mm_alloc_heap *heap) {
  assert(heap->hart == get_hart_locals(hartlock)->hart);
  assert(0 < size && size <= 1024);

  // Compute the index into pages_direct.
  usize pages_direct_index = pages_direct_index_of_size(size);

  // Get a pointer to a page to try.
  struct mm_alloc_page *page = heap->pages_direct[pages_direct_index];
  assert(page);

  // Try to pop an item from the free list. If we fail, bail out to the
  // generic routine.
  struct mm_alloc_block *block = page_free_pop(page);
  if (!block)
    return alloc_generic(size, hartlock, heap);

  // Increment the counter of used objects.
  page->used_blocks++;

  // Return the popped block.
  return block;
}

static void *alloc_generic_from_page(struct mm_alloc_page *page,
                                     struct mm_alloc_heap *heap) {
  assert(!list_is_empty(&page->list));
  assert(page_segment(page)->used_pages);

  // Try to pop an item from the free list. This function only gets called when
  // we know there is free space in the page, so we don't need to handle
  // allocation failure.
  struct mm_alloc_block *block = page_free_pop(page);
  assert(block);

  // Increment the counter of used objects and return.
  page->used_blocks++;
  return block;
}

static void *alloc_huge(usize size, struct hartlock *hartlock, struct mm_alloc_heap *heap) {
  assert(size_is_huge(size));

  // Every huge object goes in its own page, so allocate one and use it
  // directly.
  struct mm_alloc_page *page = page_new_huge(hartlock, heap, size);
  if (!page)
    return nullptr;
  return alloc_generic_from_page(page, heap);
}

void *alloc_generic(usize size, struct hartlock *hartlock, struct mm_alloc_heap *heap) {
  assert(heap->hart == get_hart_locals(hartlock)->hart);
  assert(size);

  // Go through the delayed free list to free everything.
  while (heap->delayed_free) {
    struct mm_alloc_block **block =
        (struct mm_alloc_block **)heap->delayed_free;
    heap->delayed_free = *block;
    free(block);
  }

  // Huge objects get handled separately.
  if (size_is_huge(size))
    return alloc_huge(size, hartlock, heap);

  // Check every page of the size class for free objects.
  usize size_class = size_class_of_size(size);
  struct mm_alloc_page *page;
  struct list_head *page_head = heap->pages[size_class].next;
  while (page_head != &heap->pages[size_class]) {
    page = container_of(page_head, struct mm_alloc_page, list);
    page_head = page_head->next;

    // Free the page if it's empty.
    if (page_is_empty(page)) {
      page_free(page, heap);
      continue;
    }

    // Collect free objects in the page.
    page_collect(page);

    // If there are any free objects, use them.
    if (!page_is_full(page))
      return alloc_generic_from_page(page, heap);

    // Otherwise, move the page to the full pages list, so we don't re-traverse
    // it until something got freed.
    //
    // TODO: Race here? If between page_collect() and here, all the objects in
    // the page get remotely freed, none of them will ever end up in the delayed
    // free list, so we'll never re-traverse it.
    list_remove(&page->list);
    list_push(&heap->full_pages, &page->list);
    atomic uaddr *remote = (atomic uaddr *)&page->remote;
    atomic_fetch_or(remote, 1);
    page->in_full_list = true;
  }

  // At this point, we know that none of the pages have free objects. Allocate a
  // new one.
  assert(list_is_empty(&heap->pages[size_class]));
  if (size_is_small(size))
    page = page_new_small(hartlock, heap, size_class);
  else
    page = page_new_large(hartlock, heap, size_class);

  // If we couldn't allocate a new page, we're out of memory.
  if (!page)
    return nullptr;

  // If we could, use it.
  return alloc_generic_from_page(page, heap);
}

void *realloc(void *ptr, usize new_size) {
  struct mm_alloc_page *page = page_of_ptr((uptr)ptr);
  usize new_size_class = size_class_of_size(new_size);
  if (page->size_class == new_size_class) {
    // We're already the right size class, so just return the same pointer.
    return ptr;
  } else {
    usize old_size = size_of_size_class(page->size_class);
    void *out = alloc(new_size);
    if (!out)
      return nullptr;
    memcpy(out, ptr, old_size);
    free(ptr);
    return out;
  }
}
