/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__ALIGN_H
#define UKO_OS_KERNEL__ALIGN_H 1

#include <physical.h>

static inline uaddr uaddr_align_down(uaddr addr, usize bits) {
  uaddr low_mask = ((uaddr)1 << bits) - 1;
  return addr & ~low_mask;
}

static inline uaddr uaddr_align_up(uaddr addr, usize bits) {
  return uaddr_align_down(addr + (((uaddr)1 << bits) - 1), bits);
}

#define align_down(ADDR, BITS)                                                 \
  (_Generic(ADDR, paddr: paddr_align_down, uaddr: uaddr_align_down)(ADDR, BITS))
#define align_up(ADDR, BITS)                                                   \
  (_Generic(ADDR, paddr: paddr_align_up, uaddr: uaddr_align_up)(ADDR, BITS))

#endif // UKO_OS_KERNEL__ALIGN_H
