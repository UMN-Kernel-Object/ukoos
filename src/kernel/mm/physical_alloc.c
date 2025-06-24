#include <mm/physical_alloc.h>
#include <panic.h>
#include <physical.h>

// A singly linked list of allocatable chunks of physical memory. The first word
// of every chunk is the next pointer, the second word is the number of pages in
// the chunk.
static paddr init_physical_allocator = {0};

void mm_init_add_physical_chunk(paddr start, paddr end) {
  assert(!start.offset);
  assert(!end.offset);

  physical_write_u64le(paddr_offset(start, 0),
                       paddr_to_bits(init_physical_allocator));
  physical_write_u64le(paddr_offset(start, sizeof(paddr)),
                       (paddr_to_bits(end) - paddr_to_bits(start)) >> 12);
  init_physical_allocator = start;
}

void mm_init_physical(paddr devicetree_start, paddr devicetree_end,
                      paddr kernel_start, paddr kernel_end) {
  TODO();
}
