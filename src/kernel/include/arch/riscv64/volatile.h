/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__ARCH_RISCV64_VOLATILE_H
#define UKO_OS_KERNEL__ARCH_RISCV64_VOLATILE_H 1

/**
 * TODO:
 *
 * - GCC emits extra `andi a0, a0, 0xff`s on each of the _read_once_* functions.
 *   I assume this is because we don't have a way to promise to the compiler
 *   that we zeroed the top bits. We should fix this.
 */

#include <types.h>

static inline u8 _read_once_u8(u8 *ptr) {
  u8 out;
  __asm__("lbu %0, %1" : "=r"(out) : "m"(*ptr) : "memory");
  return out;
}

static inline u16 _read_once_u16(u16 *ptr) {
  u16 out;
  __asm__("lhu %0, %1" : "=r"(out) : "m"(*ptr) : "memory");
  return out;
}

static inline u32 _read_once_u32(u32 *ptr) {
  u32 out;
  __asm__("lwu %0, %1" : "=r"(out) : "m"(*ptr) : "memory");
  return out;
}

static inline u64 _read_once_u64(u64 *ptr) {
  u64 out;
  __asm__("ld %0, %1" : "=r"(out) : "m"(*ptr) : "memory");
  return out;
}

static inline void _write_once_u8(u8 *ptr, u8 val) {
  __asm__("sb %0, %1" : : "r"(val), "m"(*ptr) : "memory");
}

static inline void _write_once_u16(u16 *ptr, u16 val) {
  __asm__("sh %0, %1" : : "r"(val), "m"(*ptr) : "memory");
}

static inline void _write_once_u32(u32 *ptr, u32 val) {
  __asm__("sw %0, %1" : : "r"(val), "m"(*ptr) : "memory");
}

static inline void _write_once_u64(u64 *ptr, u64 val) {
  __asm__("sd %0, %1" : : "r"(val), "m"(*ptr) : "memory");
}

#endif // UKO_OS_KERNEL__ARCH_RISCV64_VOLATILE_H
