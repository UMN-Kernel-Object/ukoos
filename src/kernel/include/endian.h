/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__ENDIAN_H
#define UKO_OS_KERNEL__ENDIAN_H 1

#include <types.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

static inline u16 little_to_native_16(u16 value) { return value; }
static inline u32 little_to_native_32(u32 value) { return value; }
static inline u64 little_to_native_64(u64 value) { return value; }

static inline u16 big_to_native_16(u16 value) { return bswap(value); }
static inline u32 big_to_native_32(u32 value) { return bswap(value); }
static inline u64 big_to_native_64(u64 value) { return bswap(value); }

static inline u16 native_to_little_16(u16 value) { return value; }
static inline u32 native_to_little_32(u32 value) { return value; }
static inline u64 native_to_little_64(u64 value) { return value; }

static inline u16 native_to_big_16(u16 value) { return bswap(value); }
static inline u32 native_to_big_32(u32 value) { return bswap(value); }
static inline u64 native_to_big_64(u64 value) { return bswap(value); }

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

static inline u16 little_to_native_16(u16 value) { return bswap(value); }
static inline u32 little_to_native_32(u32 value) { return bswap(value); }
static inline u64 little_to_native_64(u64 value) { return bswap(value); }

static inline u16 big_to_native_16(u16 value) { return value; }
static inline u32 big_to_native_32(u32 value) { return value; }
static inline u64 big_to_native_64(u64 value) { return value; }

static inline u16 native_to_little_16(u16 value) { return bswap(value); }
static inline u32 native_to_little_32(u32 value) { return bswap(value); }
static inline u64 native_to_little_64(u64 value) { return bswap(value); }

static inline u16 native_to_big_16(u16 value) { return value; }
static inline u32 native_to_big_32(u32 value) { return value; }
static inline u64 native_to_big_64(u64 value) { return value; }

#else
#error Unsupported endianness
#endif

#define little_to_native(X)                                                    \
  (_Generic(X,                                                                 \
       u16: little_to_native_16,                                               \
       u32: little_to_native_32,                                               \
       u64: little_to_native_64)(X))
#define big_to_native(X)                                                       \
  (_Generic(X,                                                                 \
       u16: big_to_native_16,                                                  \
       u32: big_to_native_32,                                                  \
       u64: big_to_native_64)(X))
#define native_to_little(X)                                                    \
  (_Generic(X,                                                                 \
       u16: native_to_little_16,                                               \
       u32: native_to_little_32,                                               \
       u64: native_to_little_64)(X))
#define native_to_big(X)                                                       \
  (_Generic(X,                                                                 \
       u16: native_to_big_16,                                                  \
       u32: native_to_big_32,                                                  \
       u64: native_to_big_64)(X))

#endif // UKO_OS_KERNEL__ENDIAN_H
