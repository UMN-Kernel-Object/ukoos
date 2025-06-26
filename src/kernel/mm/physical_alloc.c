#include <mm/physical_alloc.h>
#include <panic.h>
#include <physical.h>
#include <print.h>

// A singly linked list of allocatable chunks of physical memory. The first word
// of every chunk is the next pointer, the second word is the number of pages in
// the chunk.
static paddr init_physical_allocator = {0};

void mm_init_add_physical_chunk(paddr start, paddr end) {
  assert(!start.offset);
  assert(!end.offset);

  physical_write_u64le(start, bits_of_paddr(init_physical_allocator));
  physical_write_u64le(paddr_offset(start, sizeof(paddr)),
                       (bits_of_paddr(end) - bits_of_paddr(start)) >> 12);
  init_physical_allocator = start;
}

static void union_range(paddr *start1, paddr *end1, paddr start2, paddr end2) {
  if (bits_of_paddr(start2) < bits_of_paddr(*start1))
    *start1 = start2;
  if (bits_of_paddr(end2) > bits_of_paddr(*end1))
    *end1 = end2;
}

void mm_init_physical(paddr devicetree_start, paddr devicetree_end,
                      paddr kernel_start, paddr kernel_end) {
  // Compute the range we need to handle allocating physical memory from, and
  // log the chunks of memory we've been given.
  devicetree_start = paddr_align_up(devicetree_start, 12);
  devicetree_end = paddr_align_down(devicetree_end, 12);
  kernel_start = paddr_align_up(kernel_start, 12);
  kernel_end = paddr_align_down(kernel_end, 12);
  paddr all_physical_start = devicetree_start,
        all_physical_end = devicetree_end;
  union_range(&all_physical_start, &all_physical_end, devicetree_start,
              devicetree_end);
  print("Allocatable memory chunks:");
  for (paddr next_chunk = init_physical_allocator; bits_of_paddr(next_chunk);
       next_chunk = paddr_of_bits(physical_read_u64le(next_chunk))) {
    paddr chunk_start = next_chunk;
    u64 length = physical_read_u64le(paddr_offset(next_chunk, sizeof(paddr)));
    paddr chunk_end = paddr_offset(chunk_start, length << 12);
    union_range(&all_physical_start, &all_physical_end, chunk_start, chunk_end);
    print("  {paddr}-{paddr}", chunk_start, chunk_end);
  }
  print("Preparing the physical allocator to handle {paddr}-{paddr}...",
        all_physical_start, all_physical_end);

  TODO();
}
