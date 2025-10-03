/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MINMAX_H
#define UKO_OS_KERNEL__MINMAX_H 1

#include <macro_utils.h>
#include <types.h>

#define __DEFINE_MINMAX_FOR_TYPE(TYPE)                                         \
  [[gnu::const]] static inline TYPE max_##TYPE(TYPE lhs, TYPE rhs) {           \
    return (lhs > rhs) ? lhs : rhs;                                            \
  }                                                                            \
                                                                               \
  [[gnu::const]] static inline TYPE min_##TYPE(TYPE lhs, TYPE rhs) {           \
    return (lhs < rhs) ? lhs : rhs;                                            \
  }

__DEFINE_MINMAX_FOR_TYPE(i8)
__DEFINE_MINMAX_FOR_TYPE(u8)
__DEFINE_MINMAX_FOR_TYPE(i16)
__DEFINE_MINMAX_FOR_TYPE(u16)
__DEFINE_MINMAX_FOR_TYPE(i32)
__DEFINE_MINMAX_FOR_TYPE(u32)
__DEFINE_MINMAX_FOR_TYPE(i64)
__DEFINE_MINMAX_FOR_TYPE(u64)

#define max(LHS, RHS)                                                          \
  ({                                                                           \
    const auto __max_LHS = (LHS);                                              \
    const auto __max_RHS = (RHS);                                              \
    static_assert(__HACKY_INTEGRAL_TYPEID(__max_LHS) ==                        \
                      __HACKY_INTEGRAL_TYPEID(__max_RHS),                      \
                  "cannot compute max(" #LHS ", " #RHS                         \
                  "), when the two arguments have different types");           \
    (__max_LHS > __max_RHS) ? __max_LHS : __max_RHS;                           \
  })
#define min(LHS, RHS)                                                          \
  ({                                                                           \
    const auto __min_LHS = (LHS);                                              \
    const auto __min_RHS = (RHS);                                              \
    static_assert(__HACKY_INTEGRAL_TYPEID(__min_LHS) ==                        \
                      __HACKY_INTEGRAL_TYPEID(__min_RHS),                      \
                  "cannot compute min(" #LHS ", " #RHS                         \
                  "), when the two arguments have different types");           \
    (__min_LHS < __min_RHS) ? __min_LHS : __min_RHS;                           \
  })

#endif // UKO_OS_KERNEL__MINMAX_H
