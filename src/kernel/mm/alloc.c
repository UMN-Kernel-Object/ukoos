/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hart_locals.h>
#include <minmax.h>
#include <mm/alloc.h>
#include <panic.h>
#include <random.h>

// TODO: DEBUG
#include <print.h>

static usize size_class_of_size(usize size) {
  assert(size <= 512 * 1024);
  if (size < 8)
    return 0;
  usize log2_size = stdc_trailing_zeros(stdc_bit_ceil(size));
  assert(log2_size >= 3);
  usize size_class = log2_size - 3;
  assert(size_class < 17);
  return size_class;
}

static usize size_of_size_class(usize size_class) {
  assert(size_class < 17);
  return (usize)8 << size_class;
}

/**
 * Given a pointer into the segment metadata or a pointer to the start of an
 * allocation inside a segment, returns the address of the segment.
 *
 * Returns `nullptr` when given `nullptr`.
 */
static struct mm_alloc_segment *segment_of_ptr(void *ptr) {
  uaddr ptr_addr = addr_of_ptr(ptr);
  uaddr segment_addr = ptr_addr & ~(((usize)1 << MM_ALLOC_SEGMENT_SHIFT) - 1);
  u8 *byte_ptr = ptr;
  // TODO: This is broken under strict provenance. Perhaps we should store a
  // pointer that is valid for the entire range of VA space the allocator
  // claims, and derive this pointer from that? That gives us the right place to
  // check bounds and alignment, too.
  return (struct mm_alloc_segment *)&byte_ptr[(iaddr)(segment_addr - ptr_addr)];
}

/**
 * Given a pointer to the start of an allocation inside a page, returns the
 * address of the page metadata.
 *
 * Returns `nullptr` when given `nullptr`.
 */
static struct mm_alloc_page *page_of_ptr(void *ptr) {
  struct mm_alloc_segment *segment = segment_of_ptr(ptr);
  if (!segment)
    return nullptr;
  const isize ptr_offset_in_segment = ptr - (void *)segment;
  const usize page_index = (usize)ptr_offset_in_segment >> segment->page_shift;
  return &segment->pages[page_index];
}

/**
 * Computes the bounds of the allocations for a page.
 */
static void get_page_bounds(struct mm_alloc_page *page, uptr *out_start,
                            uptr *out_end) {
  struct mm_alloc_segment *segment = segment_of_ptr(page);
  isize page_index = page - &segment->pages[0];
  assert(0 <= page_index && page_index < 64,
         "page index {isize} was somehow out of bounds?", page_index);
  *out_start = (uptr)segment + ((usize)page_index << 16);
  *out_end = *out_start + (1 << 16);

  // The first page in a segment has its data shortened by two pages, so we can
  // put a guard page between the metadata and data.
  if (page_index == 0)
    *out_start += 2 << 12;
}

/**
 * Pops a block from `page->free`, returning it. If `page->free` is empty,
 * returns `nullptr`.
 */
static struct mm_alloc_block *page_free_pop(struct mm_alloc_page *page) {
  struct mm_alloc_block *block =
      ptr_with_addr(page, page->free ^ page->xor_cookie);
  if (block) {
    uptr page_start, page_end;
    get_page_bounds(page, &page_start, &page_end);
    assert(addr_of_ptr(page_start) <= addr_of_ptr(block) &&
           addr_of_ptr(block) < addr_of_ptr(page_end));

    assert((block->next & 7) == 7);
    assert((page->free & 7) == 7);

    page->free = block->next;
    block->next = 0;
  }

  assert((page->free & 7) == 7);

  return block;
}

/**
 * Pushes a block to `page->free`.
 */
static void page_free_push(struct mm_alloc_page *page,
                           struct mm_alloc_block *block, usize block_size) {
  assert((page->free & 7) == 7);

  block->next = page->free;
  bzero((void *)block + 8, block_size - 8);
  page->free = addr_of_ptr(block) ^ page->xor_cookie;

  assert((block->next & 7) == 7);
  assert((page->free & 7) == 7);
}

/**
 * Pushes a block to `page->local_free`.
 */
static void page_local_free_push(struct mm_alloc_page *page,
                                 struct mm_alloc_block *block) {
  assert((page->local_free & 7) == 7);

  block->next = page->local_free;
  page->local_free = addr_of_ptr(block) ^ page->xor_cookie;

  assert((block->next & 7) == 7);
  assert((page->local_free & 7) == 7);
}

/**
 * Recreates the pages_direct entries for the size class.
 */
static void update_pages_direct(struct mm_alloc_heap *heap, usize size_class) {
  usize size = size_of_size_class(size_class);
  if (size <= 1024) {
    assert(!list_is_empty(&heap->pages[size_class]));
    struct mm_alloc_page *page =
        container_of(heap->pages[size_class].next, struct mm_alloc_page, list);

    if (size_class == 0) {
      heap->pages_direct[0] = page;
    } else {
      usize i = (size_of_size_class(size_class - 1) >> 3);
      usize j = (size_of_size_class(size_class) >> 3);
      while (i < j) {
        assert(size_class_of_size((i + 1) << 3) == size_class);
        heap->pages_direct[i++] = page;
      }
    }
  }
}

/**
 * Allocates a new page for the size class, pushing it to the end of the
 * appropriate `pages` list.
 */
static void new_page_for_small_size_class(struct mm_alloc_heap *heap,
                                          usize size_class) {
  assert(size_class < 10);
  usize block_size = size_of_size_class(size_class);
  assert(size_class_of_size(block_size) == size_class);

  // If there aren't any unused pages, allocate a new segment.
  if (list_is_empty(&heap->unused_pages)) {
    TODO("allocate a new segment");
  }

  // Try to find a page in the unused_pages list.
  struct mm_alloc_page *page =
      container_of(list_shift(&heap->unused_pages), struct mm_alloc_page, list);

  // Initialize the page's XOR cookie.
  //
  // As a hack for debugging: blocks in the linked lists will always be aligned
  // to at least 8 bytes, so the low 3 bits will always be equal to those bits
  // in the XOR cookie. Since we're not getting much actual security from them,
  // we can at least use them to find bugs.
  //
  // We always set those bits to be high in the XOR cookie, so we can use
  // "anything in any linked list should have those bits set" as an invariant.
  page->xor_cookie = random() | 7;

  // Clear out the other fields to indicate that the page is empty.
  page->free = page->xor_cookie; // We initialize the free list below.
  page->local_free = page->xor_cookie;
  page->remote = (union mm_alloc_page_remote){
      .needs_delayed_free = false,
      .remote_free = 0,
  };
  page->used_blocks = 0;
  page->remote_free_length = 0;
  page->size_class = size_class & 0x7f;
  page->in_full_list = false;

  // Figure out the range of addresses that need to be initialized.
  uptr page_start, page_end;
  get_page_bounds(page, &page_start, &page_end);

  // Randomly initialize the free list. For efficiency's sake, the free list is
  // not perfectly shuffled; instead, we keep a buffer of 16 blocks we might add
  // to the free list, and insert them in random order, adding a new block to
  // the buffer to keep it full.
  struct mm_alloc_block *blocks[16];
  for (usize i = 0; i < 16; i++) {
    blocks[i] = (struct mm_alloc_block *)page_start;
    page_start += block_size;
  }
  assert(page_start < page_end);
  while (page_start != page_end) {
    assert(page_start < page_end);

    // Choose a random block and remove it from the array, replacing it with a
    // new one.
    usize i = random() & 0xf;
    page_free_push(page, blocks[i], block_size);
    blocks[i] = (struct mm_alloc_block *)page_start;
    page_start += block_size;
  }

  // Push the remaining blocks.
  for (usize i = 0; i < 16; i++)
    page_free_push(page, blocks[i], block_size);

  // Push the page to the end of the appropriate list in `heap->pages`.
  list_push(&heap->pages[size_class], &page->list);

  // If the page is small enough to have `pages_direct` entries, make them too.
  update_pages_direct(heap, size_class);
}

static struct mm_alloc_page *page_alloc(usize size_class,
                                        struct mm_alloc_heap *heap) {
  TODO("page_alloc({usize}, {uptr})", size_class, heap);
}

static void page_free(struct mm_alloc_page *page, struct mm_alloc_heap *heap) {
  assert(page->used_blocks == page->remote_free_length);

  // Dirty trick: don't actually free the page if it's the only one in the
  // `pages` list.
  if (page->list.prev == page->list.next)
    return;

  // Clear the contents of the page.
  uptr page_start, page_end;
  get_page_bounds(page, &page_start, &page_end);
  bzero((void *)page_start, page_end - page_start);

  // Unlink the page from its list.
  list_remove(&page->list);

  // Add the page to the heap's `unused_pages` list.
  list_push(&heap->unused_pages, &page->list);

  // If the page's size was such that it could've been in the pages_direct
  // array, replace those entries with whatever the new head of the list is.
  update_pages_direct(heap, page->size_class);
}

static void *alloc_generic(usize size, struct mm_alloc_heap *heap) {
  assert(size != 0);
  // Huge objects get handled separately, since they always get dedicated
  // segments.
  if (size <= 512 * 1024) {
    usize size_class = size_class_of_size(size);

    // TODO: If we support deferred freeing, this is where that would go.

    struct mm_alloc_page *page;
    for (struct list_head *page_head = heap->pages[size_class].next;
         page_head != &heap->pages[size_class]; page_head = page_head->next) {
      page = container_of(page_head, struct mm_alloc_page, list);

      // Collect free objects in the page.
      TODO("page_collect {uptr}", page);

      // Free the page if it's empty.
      if (page->used_blocks == page->remote_free_length)
        page_free(page, heap);

      // If there are any free objects, go back to generic allocation.
      TODO("alloc {uptr}", page);
    }

    // We couldn't find any pages with free memory, so grab a page.
    page = page_alloc(size_class, heap);
    TODO("alloc_generic {usize} {usize}", size_class, size);
  } else {
    TODO("alloc_generic huge {usize}", size);
  }
}

[[gnu::alloc_size(1), gnu::malloc, gnu::malloc(free, 1), gnu::nonnull(2),
  nodiscard]] static void *
alloc_small(usize size, struct mm_alloc_heap *heap) {
  assert(0 < size && size <= 1024);

  // Round up the size; the smallest size class is 8 bytes.
  size = (size + 7) & ~(usize)7;

  // Compute the index into pages_direct.
  usize pages_direct_index = (size >> 3) - 1;
  assert(pages_direct_index < 128);

  // Get a pointer to a page to try.
  struct mm_alloc_page *page = heap->pages_direct[pages_direct_index];
  assert(page);

  // Try to pop an item from the free list. If we fail, bail out to the
  // generic routine.
  struct mm_alloc_block *block = page_free_pop(page);
  if (!block)
    return alloc_generic(size, heap);

  // Increment the counter of used objects.
  page->used_blocks++;

  // Return the popped block.
  return block;
}

[[gnu::alloc_size(1), gnu::malloc, gnu::malloc(free, 1), nodiscard]] void *
alloc(usize size) {
  struct mm_alloc_heap *heap = get_hart_locals()->heap;
  if (size == 0)
    return alloc_small(1, heap);
  else if (size <= 1024)
    return alloc_small(size, heap);
  else
    return alloc_generic(size, heap);
}

void free(void *ptr) {
  struct mm_alloc_segment *segment = segment_of_ptr(ptr);
  struct mm_alloc_page *page = page_of_ptr(ptr);
  if (segment->hart_id == get_hart_locals()->hart_id) {
    page_local_free_push(page, ptr);
    page->used_blocks--;
    if (page->used_blocks == page->remote_free_length)
      page_free(page, get_hart_locals()->heap);
  } else {
    TODO("remote free");
  }
}

void *realloc(void *ptr, usize new_size) {
  struct mm_alloc_page *page = page_of_ptr(ptr);
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

static void segment_init_small(struct mm_alloc_heap *heap,
                               struct mm_alloc_segment *segment) {
  assert(segment_of_ptr(segment) == segment);
  assert(heap->hart_id == get_hart_locals()->hart_id);
  *segment = (struct mm_alloc_segment){
      .hart_id = heap->hart_id,
      .used_pages = 0,
      .page_shift = MM_ALLOC_PAGE_SMALL_SHIFT,
  };
  for (usize i = 0; i < 64; i++) {
    segment->pages[i] = (struct mm_alloc_page){
        .list = LIST_INIT(segment->pages[i].list),
        .free = 0,
        .local_free = 0,
        .remote = (union mm_alloc_page_remote){.bits = 0},
        .xor_cookie = 0,
        .used_blocks = 0,
        .remote_free_length = 0,
        .size_class = 0,
        .in_full_list = false,
    };
    list_push(&heap->unused_pages, &segment->pages[i].list);
  }
}

void mm_alloc_boothart_heap_init(struct mm_alloc_heap *heap,
                                 struct mm_alloc_segment *segment) {
  // Initialize the heap. pages_direct is full of null pointers, though,
  // which is normally illegal -- we fix that below.
  *heap = (struct mm_alloc_heap){
      .hart_id = get_hart_locals()->hart_id,
      .pages_direct = {0},
      .pages = {0}, // initialized below
      .unused_pages = LIST_INIT(heap->unused_pages),
      .full_pages = LIST_INIT(heap->full_pages),
      .delayed_free = nullptr,
  };
  for (usize i = 0; i < sizeof(heap->pages) / sizeof(heap->pages[0]); i++)
    heap->pages[i] = LIST_INIT(heap->pages[i]);

  // Initialize the first segment.
  segment_init_small(heap, segment);

  // Allocate pages of the right size classes to initialize the pages_direct
  // array.
  for (usize size_class = 0; size_class < 8; size_class++)
    new_page_for_small_size_class(heap, size_class);

  // Check that we did actually initialize the entire pages_direct array.
  for (usize i = 0; i < 128; i++) {
    usize size_class = size_class_of_size((i + 1) * 8);
    assert(heap->pages_direct[i], "i={usize}", i);
    assert(heap->pages_direct[i]->size_class == size_class);
  }
}
