#include <align.h>
#include <mm/paging.h>
#include <mm/physical_alloc.h>
#include <panic.h>
#include <physical.h>
#include <print.h>

/**
 * The physical memory allocator. This uses a single large buddy allocator that
 * covers all the memory we might allocate from.
 */

struct physical_free_list {
  struct physical_free_list *next;
  // The length of the free list, including this link.
  usize length;
};

// A singly linked list of allocatable chunks of physical memory. The first word
// of every chunk is the next pointer, the second word is the number of pages in
// the chunk.
static paddr init_physical_allocator = {0};

// The start and end address of the range of physical memory the buddy allocator
// covers.
static paddr physical_buddy_start = {0}, physical_buddy_end = {0};

// The number of size classes in the buddy allocator.
static usize physical_buddy_size_class_count = 0;

// The buddy free lists.
static struct physical_free_list **physical_free_lists = nullptr;

// The buddy bitmap.
static u8 *physical_bitmap = nullptr;

void mm_init_add_physical_chunk(paddr start, paddr end) {
  assert(!start.offset);
  assert(!end.offset);

  physical_write_u64le(start, bits_of_paddr(init_physical_allocator));
  physical_write_u64le(paddr_offset(start, sizeof(paddr)),
                       paddr_diff(end, start) >> 12);
  init_physical_allocator = start;
}

static void union_range(paddr *start1, paddr *end1, paddr start2, paddr end2) {
  if (bits_of_paddr(start2) < bits_of_paddr(*start1))
    *start1 = start2;
  if (bits_of_paddr(end2) > bits_of_paddr(*end1))
    *end1 = end2;
}

static void mm_init_add_chunk_to_buddy(paddr chunk_start, paddr chunk_end) {
  usize start = paddr_diff(chunk_start, physical_buddy_start),
        end = paddr_diff(chunk_end, physical_buddy_start);

  print("TODO: mm_init_add_chunk_to_buddy({paddr}, {paddr})", chunk_start,
        chunk_end);
  print("                                ({uptr}, {uptr})", (uptr)start,
        (uptr)end);
}

void mm_init_physical(paddr devicetree_start, paddr devicetree_end,
                      paddr kernel_start, paddr kernel_end, uptr *free_va_start,
                      uptr *free_va_end) {
  // Compute the range we need to handle allocating physical memory from, and
  // log the chunks of memory we've been given.
  devicetree_start = paddr_align_up(devicetree_start, 12);
  devicetree_end = paddr_align_down(devicetree_end, 12);
  kernel_start = paddr_align_up(kernel_start, 12);
  kernel_end = paddr_align_down(kernel_end, 12);
  physical_buddy_start = devicetree_start;
  physical_buddy_end = devicetree_end;
  union_range(&physical_buddy_start, &physical_buddy_end, devicetree_start,
              devicetree_end);
  print("Allocatable memory chunks:");
  for (paddr next_chunk = init_physical_allocator; bits_of_paddr(next_chunk);
       next_chunk = paddr_of_bits(physical_read_u64le(next_chunk))) {
    paddr chunk_start = next_chunk;
    u64 length = physical_read_u64le(paddr_offset(next_chunk, sizeof(paddr)));
    paddr chunk_end = paddr_offset(chunk_start, length << 12);
    union_range(&physical_buddy_start, &physical_buddy_end, chunk_start,
                chunk_end);
    print("  {paddr}-{paddr}", chunk_start, chunk_end);
  }
  print("Preparing the physical allocator to handle {paddr}-{paddr}...",
        physical_buddy_start, physical_buddy_end);

  // Round the free virtual address range to page boundaries.
  *free_va_start = align_up(*free_va_start, 12);
  *free_va_end = align_down(*free_va_end, 12);

  // We round up the area we need to cover to the next power of two, to make the
  // math easy. Use it to update the end address.
  const usize physical_buddy_page_count =
      stdc_bit_ceil(paddr_diff(physical_buddy_end, physical_buddy_start) >> 12);
  physical_buddy_end =
      paddr_offset(physical_buddy_start, physical_buddy_page_count << 12);

  // If we have fewer than four pages, we won't have enough to boot, and we'd
  // need to do more math below.
  if (physical_buddy_page_count < 4)
    panic("Not enough memory to boot!");

  // Compute the number of size classes (i.e., the number of free lists). We
  // have one for each level of branch nodes, plus one for the leaf nodes.
  physical_buddy_size_class_count =
      stdc_trailing_zeros(physical_buddy_page_count) + 1;

  // Compute how many bytes we need in the bitmap. We need two bits per page
  // (since a complete binary tree with n leaves has 2n-1 nodes, we ignore the
  // -1 bit), so one byte for every four pages.
  const usize physical_buddy_bytes = physical_buddy_page_count / 4;

  // Compute the number of pages we need to store the book-keeping structures
  // for the physical allocator. These structures are the free lists plus the
  // bitmap.
  const usize physical_buddy_bookkeeping_bytes =
      physical_buddy_size_class_count * sizeof(struct physical_free_list *) +
      physical_buddy_bytes;
  const usize physical_buddy_bookkeeping_pages =
      align_up(physical_buddy_bookkeeping_bytes, 12) >> 12;

  // Check that we have that number of pages available at the start of the free
  // virtual address space.
  const uptr old_free_va_start = *free_va_start;
  const uptr new_free_va_start =
      old_free_va_start + (physical_buddy_bookkeeping_pages << 12);
  if (new_free_va_start >= *free_va_end)
    panic("Not enough free virtual memory to make allocator book-keeping "
          "structures");

  // Map physical pages to back each of these.
  //
  // Since pages with higher addresses (within a memory range) should get pushed
  // to the init_physical_allocator last, and therefore be at the top, we can
  // take pages off the end by removing the last page from the top item of
  // init_physical_allocator.
  //
  // It's not clear whether this is more useful than taking them from the start,
  // but at least we're not making a random hole in the middle.
  for (uptr va = old_free_va_start; va != new_free_va_start; va += 4096) {
    paddr pa;
    if (!bits_of_paddr(init_physical_allocator))
      panic("Out of memory while building physical allocator structures!");
    u64 top_page_count = physical_read_u64le(
        paddr_offset(init_physical_allocator, sizeof(paddr)));
    if (top_page_count == 0) {
      pa = init_physical_allocator;
      init_physical_allocator =
          paddr_of_bits(physical_read_u64le(init_physical_allocator));
    } else {
      top_page_count--;
      physical_write_u64le(paddr_offset(init_physical_allocator, sizeof(paddr)),
                           top_page_count);
      pa = paddr_offset(init_physical_allocator, top_page_count << 12);
    }

    // TODO: Provide an allocator to _mm_map, so we don't blow up if we have to
    // allocate a new page table here.
    if (!_mm_map(va, pa, PGPERM_KRW))
      panic(
          "Failed to map memory while building physical allocator structures!");
  }
  mm_paging_fence();

  // Advance free_va_start and zero out the memory we allocated.
  *free_va_start = new_free_va_start;
  bzero((void *)old_free_va_start, physical_buddy_bookkeeping_pages << 12);

  // Set up the allocator's pointers to point at the virtual address space we
  // claimed.
  physical_free_lists = (struct physical_free_list **)old_free_va_start;
  physical_bitmap =
      (u8 *)(old_free_va_start + physical_buddy_size_class_count *
                                     sizeof(struct physical_free_list *));

  // Add each chunk to the buddy allocator.
  paddr chunk_start, chunk_end;
  while (bits_of_paddr(init_physical_allocator)) {
    chunk_start = init_physical_allocator;
    init_physical_allocator =
        paddr_of_bits(physical_read_u64le(init_physical_allocator));
    const u64 top_page_count =
        physical_read_u64le(paddr_offset(chunk_start, sizeof(paddr)));
    assert(top_page_count != 0);
    chunk_end = paddr_offset(chunk_start, top_page_count << 12);

    mm_init_add_chunk_to_buddy(chunk_start, chunk_end);
  }
}
