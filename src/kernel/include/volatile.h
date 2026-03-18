/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__VOLATILE_H
#define UKO_OS_KERNEL__VOLATILE_H 1

/**
 * Functions for performing "volatile-like" accesses. These do _not_ impose
 * ordering constraints (unlike actual atomics), _including with respect to each
 * other_ (making them weaker than relaxed atomics), but do execute atomically.
 */

// TODO: Figure how to #include <arch/$arch/volatile.h> instead of hard-coding
// riscv64.
#include <arch/riscv64/volatile.h>

#define READ_ONCE(PTR)                                                         \
  ({                                                                           \
    auto READ_ONCE__ptr = (PTR);                                               \
    typeof(*READ_ONCE__ptr) READ_ONCE__value;                                  \
    constexpr usize READ_ONCE__size = sizeof(READ_ONCE__value);                \
    static_assert(READ_ONCE__size == 1 || READ_ONCE__size == 2 ||              \
                      READ_ONCE__size == 4 || READ_ONCE__size == 8,            \
                  "READ_ONCE only supports {1,2,4,8}-byte types");             \
    switch (READ_ONCE__size) {                                                 \
    case sizeof(u8): {                                                         \
      u8 READ_ONCE__buf = _read_once_u8((u8 *)READ_ONCE__ptr);                 \
      memcpy(&READ_ONCE__value, &READ_ONCE__buf, READ_ONCE__size);             \
      break;                                                                   \
    }                                                                          \
    case sizeof(u16): {                                                        \
      u16 READ_ONCE__buf = _read_once_u16((u16 *)READ_ONCE__ptr);              \
      memcpy(&READ_ONCE__value, &READ_ONCE__buf, READ_ONCE__size);             \
      break;                                                                   \
    }                                                                          \
    case sizeof(u32): {                                                        \
      u32 READ_ONCE__buf = _read_once_u32((u32 *)READ_ONCE__ptr);              \
      memcpy(&READ_ONCE__value, &READ_ONCE__buf, READ_ONCE__size);             \
      break;                                                                   \
    }                                                                          \
    case sizeof(u64): {                                                        \
      u64 READ_ONCE__buf = _read_once_u64((u64 *)READ_ONCE__ptr);              \
      memcpy(&READ_ONCE__value, &READ_ONCE__buf, READ_ONCE__size);             \
      break;                                                                   \
    }                                                                          \
    default: {                                                                 \
      bzero(&READ_ONCE__value, READ_ONCE__size);                               \
      break;                                                                   \
    }                                                                          \
    }                                                                          \
    READ_ONCE__value;                                                          \
  })
#define WRITE_ONCE(PTR, VAL)                                                   \
  do {                                                                         \
    auto WRITE_ONCE__ptr = (PTR);                                              \
    auto WRITE_ONCE__value = (VAL);                                            \
    static_assert(                                                             \
        __builtin_types_compatible_p(typeof(*WRITE_ONCE__ptr),                 \
                                     typeof(WRITE_ONCE__value)),               \
        "The type of the pointer and value to WRITE_ONCE must match");         \
    constexpr usize WRITE_ONCE__size = sizeof(WRITE_ONCE__value);              \
    static_assert(WRITE_ONCE__size == 1 || WRITE_ONCE__size == 2 ||            \
                      WRITE_ONCE__size == 4 || WRITE_ONCE__size == 8,          \
                  "WRITE_ONCE only supports {1,2,4,8}-byte types");            \
    switch (WRITE_ONCE__size) {                                                \
    case sizeof(u8): {                                                         \
      u8 WRITE_ONCE__buf;                                                      \
      memcpy(&WRITE_ONCE__buf, &WRITE_ONCE__value, WRITE_ONCE__size);          \
      _write_once_u8((u8 *)WRITE_ONCE__ptr, WRITE_ONCE__buf);                  \
      break;                                                                   \
    }                                                                          \
    case sizeof(u16): {                                                        \
      u16 WRITE_ONCE__buf;                                                     \
      memcpy(&WRITE_ONCE__buf, &WRITE_ONCE__value, WRITE_ONCE__size);          \
      _write_once_u16((u16 *)WRITE_ONCE__ptr, WRITE_ONCE__buf);                \
      break;                                                                   \
    }                                                                          \
    case sizeof(u32): {                                                        \
      u32 WRITE_ONCE__buf;                                                     \
      memcpy(&WRITE_ONCE__buf, &WRITE_ONCE__value, WRITE_ONCE__size);          \
      _write_once_u32((u32 *)WRITE_ONCE__ptr, WRITE_ONCE__buf);                \
      break;                                                                   \
    }                                                                          \
    case sizeof(u64): {                                                        \
      u64 WRITE_ONCE__buf;                                                     \
      memcpy(&WRITE_ONCE__buf, &WRITE_ONCE__value, WRITE_ONCE__size);          \
      _write_once_u64((u64 *)WRITE_ONCE__ptr, WRITE_ONCE__buf);                \
      break;                                                                   \
    }                                                                          \
    default: {                                                                 \
      break;                                                                   \
    }                                                                          \
    }                                                                          \
  } while (0)

#endif // UKO_OS_KERNEL__ARCH_RISCV64_VOLATILE_H
