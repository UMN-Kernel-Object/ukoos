/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__ALIGN_H
#define UKO_OS_KERNEL__ALIGN_H 1

#include <physical.h>

static inline uaddr _align_down_uaddr(uaddr addr, usize bits) {
  uaddr low_mask = ((uaddr)1 << bits) - 1;
  return addr & ~low_mask;
}

static inline uaddr _align_up_uaddr(uaddr addr, usize bits) {
  return _align_down_uaddr(addr + (((uaddr)1 << bits) - 1), bits);
}

static inline bool _is_aligned_uaddr(uaddr addr, usize bits) {
  return _align_down_uaddr(addr, bits) == addr;
}

static inline bool _is_aligned_ptr(const void *ptr, usize bits) {
  return _is_aligned_uaddr(addr_of_ptr(ptr), bits);
}

#define align_down(ADDR, BITS)                                                 \
  (_Generic(ADDR, paddr: _align_down_paddr, uaddr: _align_down_uaddr)(ADDR,    \
                                                                      BITS))
#define align_up(ADDR, BITS)                                                   \
  (_Generic(ADDR, paddr: _align_up_paddr, uaddr: _align_up_uaddr)(ADDR, BITS))
#define is_aligned(ADDR, BITS)                                                 \
  ({                                                                           \
    const auto __is_aligned_ADDR = (ADDR);                                     \
    constexpr auto __is_aligned_ADDR_type =                                    \
        __builtin_classify_type(__is_aligned_ADDR);                            \
    const auto __is_aligned_ADDR_or_null = __builtin_choose_expr(              \
        __is_aligned_ADDR_type == __builtin_classify_type(void *),             \
        __is_aligned_ADDR, (void **)nullptr);                                  \
    const auto __is_aligned_func = _Generic(__is_aligned_ADDR,                 \
        typeof(*__is_aligned_ADDR_or_null) *: _is_aligned_ptr,                 \
        nullptr_t: _is_aligned_ptr,                                            \
        paddr: _is_aligned_paddr,                                              \
        uaddr: _is_aligned_uaddr);                                             \
    __is_aligned_func(__is_aligned_ADDR, (BITS));                              \
  })

#endif // UKO_OS_KERNEL__ALIGN_H
