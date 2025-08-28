/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__PHYSICAL_H
#define UKO_OS_KERNEL__PHYSICAL_H 1

/**
 * I/O to physical memory. The accesses performed by these functions are
 * volatile, and must be aligned.
 *
 * These functions are very inefficiently implemented -- drivers should map the
 * memory they needs to access instead. However, they do work in contexts where
 * memory cannot be allocated.
 */

#include <panic.h>

static inline paddr paddr_of_bits(unsigned long bits) {
  union {
    unsigned long bits;
    paddr paddr;
  } u;
  u.bits = bits;
  assert(u.paddr.rsvd == 0);
  return u.paddr;
}

static inline unsigned long bits_of_paddr(paddr addr) {
  union {
    unsigned long bits;
    paddr paddr;
  } u;
  assert(addr.rsvd == 0);
  u.paddr = addr;
  return u.bits;
}

static inline usize paddr_diff(paddr end, paddr start) {
  return bits_of_paddr(end) - bits_of_paddr(start);
}

static inline paddr paddr_offset(paddr paddr, usize offset) {
  return paddr_of_bits(bits_of_paddr(paddr) + offset);
}

static inline paddr paddr_align_down(paddr paddr, usize bits) {
  usize low_mask = ((usize)1 << bits) - 1;
  return paddr_of_bits(bits_of_paddr(paddr) & ~low_mask);
}

static inline paddr paddr_align_up(paddr paddr, usize bits) {
  return paddr_align_down(paddr_offset(paddr, ((usize)1 << bits) - 1), bits);
}

u8 physical_read_u8(paddr);
u16 physical_read_u16le(paddr);
u32 physical_read_u32le(paddr);
u64 physical_read_u64le(paddr);
u16 physical_read_u16be(paddr);
u32 physical_read_u32be(paddr);
u64 physical_read_u64be(paddr);

void physical_write_u8(paddr, u8);
void physical_write_u16le(paddr, u16);
void physical_write_u32le(paddr, u32);
void physical_write_u64le(paddr, u64);
void physical_write_u16be(paddr, u16);
void physical_write_u32be(paddr, u32);
void physical_write_u64be(paddr, u64);

void copy_from_physical(void *dst, paddr src, usize len);
void copy_to_physical(paddr dst, const void *src, usize len);

#endif // UKO_OS_KERNEL__PHYSICAL_H
