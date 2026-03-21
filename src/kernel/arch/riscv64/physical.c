/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <align.h>
#include <endian.h>

/**
 * The start address of the mapped region of physical memory.
 */
static constexpr uptr physical_map_start = 0xffffffc000000000;

/**
 * The end address of the mapped region of physical memory.
 */
static constexpr uptr physical_map_end = 0xffffffe000000000;

/**
 * Checks the bounds of an access to physical memory, and returns a pointer that
 * is mapped to access that region.
 */
[[gnu::alloc_size(2)]]
static void *ptr_of_paddr(paddr paddr, usize len) {
  assert(bits_of_paddr(paddr_offset(paddr, len)) <=
             (physical_map_end - physical_map_start),
         "ptr_of_paddr: address {paddr} and length {usize} out of bounds",
         paddr, len);

  return &((u8 *)physical_map_start)[bits_of_paddr(paddr)];
}

void copy_from_physical(void *dst, paddr src, usize len) {
  memcpy(dst, ptr_of_paddr(src, len), len);
}

void copy_to_physical(paddr dst, const void *src, usize len) {
  memcpy(ptr_of_paddr(dst, len), src, len);
}

void bzero_physical(paddr dst, usize len) {
  bzero(ptr_of_paddr(dst, len), len);
}

u8 physical_read_u8(paddr paddr) {
  u8 out;
  copy_from_physical(&out, paddr, 1);
  return out;
}

void physical_write_u8(paddr paddr, u8 value) {
  copy_to_physical(paddr, &value, 1);
}

#define DEFINE_PHYSICAL_RW(BITS)                                               \
  u##BITS physical_read_u##BITS##le(paddr addr) {                              \
    assert(is_aligned(addr, stdc_trailing_zeros((usize)(BITS / 8))),           \
           "paddr={paddr}", addr);                                             \
    u##BITS out;                                                               \
    copy_from_physical(&out, addr, sizeof(out));                               \
    return little_to_native(out);                                              \
  }                                                                            \
                                                                               \
  u##BITS physical_read_u##BITS##be(paddr addr) {                              \
    assert(is_aligned(addr, stdc_trailing_zeros((usize)(BITS / 8))),           \
           "paddr={paddr}", addr);                                             \
    u##BITS out;                                                               \
    copy_from_physical(&out, addr, sizeof(out));                               \
    return big_to_native(out);                                                 \
  }                                                                            \
                                                                               \
  void physical_write_u##BITS##le(paddr addr, u##BITS value) {                 \
    assert(is_aligned(addr, stdc_trailing_zeros((usize)(BITS / 8))),           \
           "paddr={paddr}", addr);                                             \
    value = native_to_little(value);                                           \
    copy_to_physical(addr, &value, sizeof(value));                             \
  }                                                                            \
                                                                               \
  void physical_write_u##BITS##be(paddr addr, u##BITS value) {                 \
    assert(is_aligned(addr, stdc_trailing_zeros((usize)(BITS / 8))),           \
           "paddr={paddr}", addr);                                             \
    value = native_to_big(value);                                              \
    copy_to_physical(addr, &value, sizeof(value));                             \
  }

DEFINE_PHYSICAL_RW(16)
DEFINE_PHYSICAL_RW(32)
DEFINE_PHYSICAL_RW(64)
