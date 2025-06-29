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
  assert(!(addr & (((u64)1 << (size_class + 12)) - 1)));

  TODO();
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

__attribute__((nonnull(1))) uptr mm_va_alloc(struct virtual_buddy *allocator,
                                             usize size) {
  assert(allocator->start < allocator->end,
         "Virtual memory allocator in an invalid state; was it initialized?");
  usize size_class = size_class_of_size(size);
  TODO("{usize}", size_class);
}

__attribute__((nonnull(1))) void mm_va_free(struct virtual_buddy *allocator,
                                            uptr ptr, usize size) {
  assert(ptr);

  usize size_class = size_class_of_size(size);
  TODO("{usize}", size_class);
}
