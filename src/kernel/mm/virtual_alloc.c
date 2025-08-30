#include <align.h>
#include <mm/virtual_alloc.h>
#include <print.h>

static struct virtual_buddy_free_list preallocated_free_links[7] = {
    // 0xffffffc000000000 - 0xffffffe000000000 (128GiB, size class 25)
    {
        .next = nullptr,
        .flags = VIRTUAL_BUDDY_FREE_LIST_BOOTSTRAP,
        .addr_hi_bits = 0xffffffc000000000 >> 12,
    },
    // 0xffffffe000000000 - 0xfffffff000000000 (64GiB, size class 24)
    {
        .next = nullptr,
        .flags = VIRTUAL_BUDDY_FREE_LIST_BOOTSTRAP,
        .addr_hi_bits = 0xffffffe000000000 >> 12,
    },
    // 0xfffffff000000000 - 0xfffffff800000000 (32GiB, size class 23)
    {
        .next = nullptr,
        .flags = VIRTUAL_BUDDY_FREE_LIST_BOOTSTRAP,
        .addr_hi_bits = 0xfffffff000000000 >> 12,
    },
    // 0xfffffff800000000 - 0xfffffffc00000000 (16GiB, size class 22)
    {
        .next = nullptr,
        .flags = VIRTUAL_BUDDY_FREE_LIST_BOOTSTRAP,
        .addr_hi_bits = 0xfffffff800000000 >> 12,
    },
    // 0xfffffffc00000000 - 0xfffffffe00000000 (8GiB, size class 21)
    {
        .next = nullptr,
        .flags = VIRTUAL_BUDDY_FREE_LIST_BOOTSTRAP,
        .addr_hi_bits = 0xfffffffc00000000 >> 12,
    },
    // 0xfffffffe00000000 - 0xffffffff00000000 (4GiB, size class 20)
    {
        .next = nullptr,
        .flags = VIRTUAL_BUDDY_FREE_LIST_BOOTSTRAP,
        .addr_hi_bits = 0xfffffffe00000000 >> 12,
    },
    // 0xffffffff00000000 - 0xffffffff80000000 (2GiB, size class 19)
    {
        .next = nullptr,
        .flags = VIRTUAL_BUDDY_FREE_LIST_BOOTSTRAP,
        .addr_hi_bits = 0xffffffff00000000 >> 12,
    },
};

static struct virtual_buddy _mm_kernel_virtual_buddy = {0};
struct virtual_buddy *const mm_kernel_virtual_buddy = &_mm_kernel_virtual_buddy;

/**
 * An index into a bitmap.
 */
struct bit_index {
  u64 bit : 3;
  u64 byte : 61;
};
static_assert(sizeof(struct bit_index) == sizeof(u64));
static_assert(alignof(struct bit_index) == alignof(u64));

static struct bit_index bit_index_of_addr(struct virtual_buddy *allocator,
                                          uptr addr, usize size_class) {
  assert(allocator->start <= (uaddr)addr);
  assert((uaddr)addr < allocator->end);
  usize start = addr - allocator->start;
  assert(!(start & (((u64)1 << (size_class + 12)) - 1)));

  union {
    u64 bits;
    struct bit_index index;
  } index = {.bits = 0};

  for (usize i = 0; i < size_class; i++)
    index.bits += 1 << (26 - i);
  index.bits += start >> (size_class + 12);
  assert(index.index.byte < sizeof(allocator->bitmap));
  return index.index;
}

static usize size_class_of_size(usize size) {
  assert(size != 0);
  usize size_class =
      stdc_trailing_zeros(stdc_bit_ceil(align_up(size, 12) >> 12));
  assert(size_class < 27);
  return size_class;
}

void mm_init_virtual(uptr free_va_start, uptr free_va_end) {
  assert(!_mm_kernel_virtual_buddy.start,
         "Kernel virtual allocator already initialized");

  print("Initializing the kernel virtual allocator...");
  _mm_kernel_virtual_buddy.start = 0xffffffc000000000;
  _mm_kernel_virtual_buddy.end = 0xffffffff80000000;
  for (usize i = 0; i < 7; i++) {
    usize size_class = 25 - i;
    _mm_kernel_virtual_buddy.free_lists[size_class] =
        &preallocated_free_links[i];
    struct bit_index index = bit_index_of_addr(
        mm_kernel_virtual_buddy,
        (uptr)preallocated_free_links[i].addr_hi_bits << 12, size_class);
    _mm_kernel_virtual_buddy.bitmap[index.byte] |= 1 << index.bit;
  }
}

static void free_virtual_buddy_free_list(struct virtual_buddy_free_list *ptr) {
  if (ptr->flags & VIRTUAL_BUDDY_FREE_LIST_BOOTSTRAP)
    return;

  TODO();
}

[[gnu::nonnull(1)]] uptr mm_va_alloc(struct virtual_buddy *allocator,
                                     usize size) {
  assert(allocator->start < allocator->end,
         "Virtual memory allocator in an invalid state; was it initialized?");

  // If there wasn't a free block in the smallest size class that would fit, we
  // need to try a larger one and split it. Find out if that's the case.
  usize actual_size_class, ideal_size_class = size_class_of_size(size);
  for (actual_size_class = ideal_size_class;
       actual_size_class < 27 && !allocator->free_lists[actual_size_class];
       actual_size_class++)
    ;

  // If the loop terminated because all free lists were empty, we must be out of
  // virtual address space; in that case, return nullptr.
  if (actual_size_class == 27)
    return 0;

  // Otherwise, there must be a free block. Pop it.
  assert(allocator->free_lists[actual_size_class]);
  struct virtual_buddy_free_list link =
      *allocator->free_lists[actual_size_class];
  free_virtual_buddy_free_list(allocator->free_lists[actual_size_class]);
  allocator->free_lists[actual_size_class] = link.next;
  uptr block_addr = (uptr)link.addr_hi_bits << 12;

  // As we walk back to our ideal size class, pop latter halves off the block.
  while (actual_size_class > ideal_size_class) {
    print("{uptr}-{uptr}", block_addr,
          block_addr + (1 << (12 + actual_size_class)));
    actual_size_class--;
  }

  TODO("{uptr}", block_addr);
}

[[gnu::nonnull(1)]] void mm_va_free(struct virtual_buddy *allocator, uptr ptr,
                                    usize size) {
  assert(ptr);

  usize size_class = size_class_of_size(size);
  TODO("{usize}", size_class);
}
