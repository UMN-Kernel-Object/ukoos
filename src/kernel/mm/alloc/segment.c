/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "segment.h"
#include "heap.h"
#include <align.h>
#include <hart_locals.h>
#include <mm/paging.h>
#include <mm/physical_alloc.h>
#include <mm/virtual_alloc.h>

struct mm_alloc_segment *segment_alloc(usize size) {
  assert(size_is_huge(size));
  size = align_up(size, 12);

  // Allocate a VMA for the segment. If we failed to (out of physical or virtual
  // memory), bubble up that OOM.
  struct vma *vma =
      vma_alloc_aligned(&kernel_virtual_allocator, size >> 12, SEGMENT_SHIFT);
  if (!vma)
    return nullptr;

  uaddr segment_start, segment_end;
  vma_bounds(vma, &segment_start, &segment_end);
  assert(is_aligned(segment_start, SEGMENT_SHIFT),
         "segment was not aligned ({uaddr})", segment_start);
  assert(segment_end - segment_start == size);

  // Allocate frames and map them to the segment.
  usize va = segment_start;
  paddr pa;
  while (va < segment_end) {
    if (!mm_alloc_physical(&pa))
      goto physical_oom;
    if (!mm_paging_map(va, pa, PGPERM_KRW)) {
      mm_free_physical(pa);
      goto physical_oom;
    }
    va += (1 << 12);
  }
  mm_paging_fence();

  // Return the segment.
  return (struct mm_alloc_segment *)segment_start;

physical_oom:
  TODO("clean up properly from OOM");
}

void segment_init_small(struct mm_alloc_segment *segment,
                        struct mm_alloc_heap *heap) {
  assert(is_aligned(segment, SEGMENT_SHIFT));
  assert(heap->hart_id == get_hart_locals()->hart_id);

  *segment = (struct mm_alloc_segment){
      .hart_id = heap->hart_id,
      .used_pages = 0,
      .page_shift = PAGE_SMALL_SHIFT,
      .pages = {},
  };
  for (usize i = 0; i < 64; i++) {
    segment->pages[i].list = LIST_INIT(segment->pages[i].list);
    heap_push_unused_page(heap, &segment->pages[i]);
  }
}

void segment_init_large(struct mm_alloc_segment *segment) {
  assert(is_aligned(segment, SEGMENT_SHIFT));

  *segment = (struct mm_alloc_segment){
      .hart_id = get_hart_locals()->hart_id,
      .used_pages = 0,
      .page_shift = PAGE_LARGE_SHIFT,
      .pages = {},
  };
  segment->pages[0].list = LIST_INIT(segment->pages[0].list);
}

void segment_init_huge(struct mm_alloc_segment *segment) {
  assert(is_aligned(segment, SEGMENT_SHIFT));

  *segment = (struct mm_alloc_segment){
      .hart_id = get_hart_locals()->hart_id,
      .used_pages = 0,
      .page_shift = PAGE_HUGE_SHIFT,
      .pages = {},
  };
  segment->pages[0].list = LIST_INIT(segment->pages[0].list);
}

struct mm_alloc_segment *segment_of_ptr(uptr ptr) {
  return (struct mm_alloc_segment *)align_down((uptr)ptr, SEGMENT_SHIFT);
}
