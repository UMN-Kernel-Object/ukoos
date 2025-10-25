/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "page.h"
#include "heap.h"
#include <align.h>
#include <random.h>

static struct mm_alloc_page *page_alloc_small(struct mm_alloc_heap *heap,
                                              usize size_class) {
  assert(size_class_is_valid(size_class));
  assert(size_class_is_small(size_class));

  // If there are no unused pages, allocate a segment for small objects. This
  // will push its pages into the unused pages list.
  if (list_is_empty(&heap->unused_pages)) {
    struct mm_alloc_segment *segment = segment_alloc(1 << SEGMENT_SHIFT);
    if (!segment)
      return nullptr;
    segment_init_small(segment, heap);
    assert(!list_is_empty(&heap->unused_pages));
  }

  // Grab an unused page and return it.
  struct mm_alloc_page *page =
      container_of(list_shift(&heap->unused_pages), struct mm_alloc_page, list);
  struct mm_alloc_segment *segment = page_segment(page);
  assert(segment->hart_id == heap->hart_id);
  assert(segment->page_shift == PAGE_SMALL_SHIFT);
  segment->used_pages++;
  return page;
}

static void page_init_common(struct mm_alloc_page *page, usize size_class) {
  assert(size_class_is_valid(size_class) || size_class == 17);

  // Initialize the page's XOR cookie.
  //
  // As a hack for debugging: blocks in the linked lists will always be aligned
  // to at least 8 bytes, so the low 3 bits will always be equal to those bits
  // in the XOR cookie. Since we're not getting much actual security from them,
  // we can at least use them to find bugs.
  //
  // We always set those bits to be high in the XOR cookie, so we can use
  // "anything in any linked list should have those bits set" as an invariant.
  page->xor_cookie = random_u64() & (((u64)1 << 56) - 1);
  page->xor_cookie |= 0b111;

  // Clear out the page's linked lists of blocks.
  page->free = block_ref_make(page, nullptr);
  page->local_free = block_ref_make(page, nullptr);
  union mm_alloc_page_remote remote;
  remote.bits = block_ref_make(page, nullptr).xorred_ptr;
  remote.needs_delayed_free = false;
  page->remote = remote;

  // Clear out the other fields.
  page->used_blocks = 0;
  page->remote_free_length = 0;
  page->size_class = size_class & 0x7f;
  page->in_full_list = false;
}

/**
 * Initializes an already-allocated page that contains small or large objects of
 * the given size class.
 */
static void page_init_small_or_large(struct mm_alloc_page *page,
                                     struct mm_alloc_heap *heap,
                                     usize size_class) {
  assert(list_is_empty(&page->list));
  usize block_size = size_of_size_class(size_class);

  // Do the common part of initialization.
  page_init_common(page, size_class);

  // Figure out the range of addresses that need to be initialized.
  uptr page_start, page_end;
  page_bounds(page, &page_start, &page_end);
  page_start = align_up(page_start, 3 + size_class);
  assert(is_aligned(page_start, 3 + size_class));
  assert(is_aligned(page_end, 3 + size_class));

  // Randomly initialize the free list. For efficiency's sake, the free list is
  // not perfectly shuffled; instead, we keep a buffer of 16 blocks we might add
  // to the free list, and insert them in random order, adding a new block to
  // the buffer to keep it full.
  struct mm_alloc_block *blocks[16];
  usize blocks_len;
  for (blocks_len = 0; blocks_len < 16; blocks_len++) {
    if (page_start == page_end)
      break;
    blocks[blocks_len] = (struct mm_alloc_block *)page_start;
    page_start += block_size;
  }
  assert(blocks_len);
  assert(page_start <= page_end);
  if (blocks_len == 16) {
    while (page_start != page_end) {
      assert(page_start < page_end);

      // Choose a random block and remove it from the array, replacing it with a
      // new one.
      //
      // TODO: The random() call here could be significantly amortized; a
      // random() call can get 32 bytes at effectively the same cost as the half
      // byte we're getting here.
      usize i = random() & 0xf;
      page_free_push(page, blocks[i], block_size);
      blocks[i] = (struct mm_alloc_block *)page_start;
      page_start += block_size;
    }

    // Push the remaining blocks.
    for (usize i = 0; i < 16; i++)
      page_free_push(page, blocks[i], block_size);
  } else {
    while (blocks_len) {
      // TODO: This can be amortized even more easily than the above, since we
      // know we're bounded at 16 nybbles, i.e. one u64.
      usize i = random() % blocks_len;
      assert(blocks[i]);
      page_free_push(page, blocks[i], block_size);
      blocks[i] = blocks[blocks_len - 1];
      blocks_len--;
    }
  }

  // Push the page to the end of the appropriate list in `heap->pages`.
  list_push(&heap->pages[size_class], &page->list);

  // If the page is small enough to have `pages_direct` entries, make them too.
  heap_update_pages_direct(heap, size_class);
}

/**
 * Initializes an already-allocated page that contains a huge object with the
 * given size.
 */
static void page_init_huge(struct mm_alloc_page *page,
                           struct mm_alloc_heap *heap, usize size) {
  assert(list_is_empty(&page->list));

  // Do the common part of initialization.
  page_init_common(page, 17);

  // Push the (only) block onto the free list.
  struct mm_alloc_segment *segment = page_segment((struct mm_alloc_page *)page);
  uptr page_start = (uptr)segment + (2 << 12);
  page_free_push(page, (struct mm_alloc_block *)page_start, align_up(size, 12));

  // Push the page to the huge pages list.
  list_push(&heap->pages[17], &page->list);
}

struct mm_alloc_page *page_new_small(struct mm_alloc_heap *heap,
                                     usize size_class) {
  assert(size_class_is_valid(size_class));
  assert(size_class_is_small(size_class));

  struct mm_alloc_page *page = page_alloc_small(heap, size_class);
  if (!page)
    return nullptr;

  page_init_small_or_large(page, heap, size_class);

  return page;
}

struct mm_alloc_page *page_new_large(struct mm_alloc_heap *heap,
                                     usize size_class) {
  assert(size_class_is_valid(size_class));
  assert(!size_class_is_small(size_class));

  struct mm_alloc_segment *segment = segment_alloc(1 << SEGMENT_SHIFT);
  if (!segment)
    return nullptr;
  segment_init_large(segment);

  struct mm_alloc_page *page = &segment->pages[0];
  page_init_small_or_large(page, heap, size_class);

  assert(segment->hart_id == heap->hart_id);
  assert(segment->page_shift == PAGE_LARGE_SHIFT);
  segment->used_pages++;
  return page;
}

struct mm_alloc_page *page_new_huge(struct mm_alloc_heap *heap, usize size) {
  assert(size_is_huge(size));

  // The segment has to be made larger by 2 pages, for the header to fit.
  struct mm_alloc_segment *segment = segment_alloc(size + (2 << 12));
  if (!segment)
    return nullptr;
  segment_init_huge(segment);

  struct mm_alloc_page *page = &segment->pages[0];
  page_init_huge(page, heap, size);

  assert(segment->hart_id == heap->hart_id);
  assert(segment->page_shift == PAGE_HUGE_SHIFT);
  segment->used_pages++;
  return page;
}

void page_free(struct mm_alloc_page *page, struct mm_alloc_heap *heap) {
  assert(page_is_empty(page));

  // Remove the page from whichever list it's in in the heap, making sure the
  // pages_direct array is maintained.
  if (size_class_is_valid(page->size_class) &&
      size_class_in_pages_direct(page->size_class)) {
    // If this is the only page in the size class, don't actually free it. If we
    // did, we'd have to allocate a new page to use for the pages_direct entries
    // anyway.
    if (page->list.prev == page->list.next)
      return;

    // Otherwise, update the pages_direct array.
    list_remove(&page->list);
    heap_update_pages_direct(heap, page->size_class);
  } else {
    list_remove(&page->list);
  }

  struct mm_alloc_segment *segment = page_segment(page);
  assert(segment->used_pages);
  if (--segment->used_pages) {
    // There are still more pages in use, so we can't free the segment, and we
    // know this is a page for small objects, so it should end up in the
    // unused_pages list.
    heap_push_unused_page(heap, page);
  } else {
    // This was the last page in use, so we can free the entire segment.
    segment_free(segment);
  }
}

struct mm_alloc_page *page_of_ptr(uptr ptr) {
  struct mm_alloc_segment *segment = segment_of_ptr(ptr);
  usize offset = addr_of_ptr(ptr) - addr_of_ptr(segment);
  usize page_index = offset >> segment->page_shift;
  if (page_index)
    assert(segment->page_shift == PAGE_SMALL_SHIFT);
  return &segment->pages[page_index];
}

void page_bounds(const struct mm_alloc_page *page, uptr *out_start,
                 uptr *out_end) {
  assert(size_class_is_valid(page->size_class));

  struct mm_alloc_segment *segment = page_segment((struct mm_alloc_page *)page);
  usize i = page_index(page);

  uptr page_start = (uptr)segment + ((usize)i << segment->page_shift);
  uptr page_end = page_start + ((usize)1 << segment->page_shift);

  // The first page in a segment has its data shortened by two pages, so we can
  // put a guard page between the metadata and data.
  if (i == 0)
    page_start += 2 << 12;

  assert(is_aligned(page_start, 12));
  assert(is_aligned(page_end, 16));
  *out_start = page_start;
  *out_end = page_end;
}

void page_collect(struct mm_alloc_page *page) {
  // Since our hart owns the local free list, we can just move it over.
  assert(!block_ref_deref(page, page->free));
  page->free = page->local_free;
  page->local_free = block_ref_make(page, nullptr);

  // Swap the remote free list with a default one.
  if (page->remote_free_length)
    TODO("swap the remote free list");
}

static void page_push(struct mm_alloc_page *page, struct mm_alloc_block *block,
                      struct mm_alloc_block_ref *list) {
  assert(page_in_bounds(page, addr_of_ptr(block)));
  assert(block_ref_is_valid(*list));
  block->next = *list;
  *list = block_ref_make(page, block);
}

void page_free_push(struct mm_alloc_page *page, struct mm_alloc_block *block,
                    usize block_size) {
  assert(block_size >= sizeof(struct mm_alloc_block));
  bzero(block, block_size);
  page_push(page, block, &page->free);
}

void page_local_free_push(struct mm_alloc_page *page,
                          struct mm_alloc_block *block) {
  assert(!page_is_empty(page));
  page_push(page, block, &page->local_free);
  page->used_blocks--;
}

usize page_index(const struct mm_alloc_page *page) {
  struct mm_alloc_segment *segment = page_segment((struct mm_alloc_page *)page);
  isize i = page - &segment->pages[0];
  assert(0 <= i && i < ARRAY_SIZE(segment->pages));
  return (usize)i;
}

bool page_in_bounds(const struct mm_alloc_page *page, uaddr addr) {
  if (page->size_class == 17)
    return addr == align_down((uptr)page, 12) + (2 << 12);

  uptr page_start, page_end;
  page_bounds(page, &page_start, &page_end);
  return (addr_of_ptr(page_start) <= addr && addr < addr_of_ptr(page_end));
}

bool page_is_empty(const struct mm_alloc_page *page) {
  return page->used_blocks == page->remote_free_length;
}

bool page_is_full(const struct mm_alloc_page *page) {
  assert(block_ref_is_valid(page->free));
  return !block_ref_deref(page, page->free);
}

struct mm_alloc_segment *page_segment(struct mm_alloc_page *page) {
  return segment_of_ptr((uptr)page);
}
