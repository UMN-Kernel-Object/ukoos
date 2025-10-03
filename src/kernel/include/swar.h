/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__SWAR_H
#define UKO_OS_KERNEL__SWAR_H 1

#include <types.h>

/**
 * Utilities for SIMD Within A Register (SWAR). These utilities are limited, but
 * portable.
 */

/**
 * An 8-element vector of `u8`s.
 */
typedef u64 u8x8;

/**
 * Returns a vector with `value` in every entry.
 */
static inline u8x8 u8x8_broadcast(u8 value) {
  u8x8 out = value;
  out |= out << 8;
  out |= out << 16;
  out |= out << 32;
  return out;
}

/**
 * Returns whether an entry in `vec` has a value of 0x00.
 */
static inline bool u8x8_contains_zero(u8x8 vec) {
  // The high bit of each entry is set if the matching entry of `vec` is 0x00,
  // or in the range [0x81, 0xff]... or if there's a borrow in.
  const u8x8 mask_81_to_00 = vec - u8x8_broadcast(0x01);

  // The high bit of each entry is set if the matching entry of `vec` is in the
  // range [0x00, 0x7f].
  const u8x8 mask_00_to_7f = ~vec;

  // The high bit of each entry is set if the matching entry of `vec` is 0x00 or
  // there's a carry in, since it must match both of the above criteria.
  //
  // The lowest set high bit corresponds to an entry of `vec` whose value is
  // 0x00, since there's no way it can have a carry in.
  const u8x8 mask_00 = mask_00_to_7f & mask_81_to_00;

  // Only the high bits of mask_00.
  const u8x8 hi_set_if_00 = mask_00 & u8x8_broadcast(0x80);
  return hi_set_if_00 != 0;
}

/**
 * Returns one plus the index of the first entry in `vec` with a value of 0x00,
 * or zero if no entry was zero.
 */
static inline usize u8x8_find_first_zero(u8x8 vec) {
  // The high bit of each entry is set if the matching entry of `vec` is 0x00,
  // or in the range [0x81, 0xff]... or if there's a borrow in.
  const u8x8 mask_81_to_00 = vec - u8x8_broadcast(0x01);

  // The high bit of each entry is set if the matching entry of `vec` is in the
  // range [0x00, 0x7f].
  const u8x8 mask_00_to_7f = ~vec;

  // The high bit of each entry is set if the matching entry of `vec` is 0x00 or
  // there's a carry in, since it must match both of the above criteria.
  //
  // The lowest set high bit corresponds to an entry of `vec` whose value is
  // 0x00, since there's no way it can have a carry in.
  const u8x8 mask_00 = mask_00_to_7f & mask_81_to_00;

  // Only the high bits of mask_00.
  const u8x8 hi_set_if_00 = mask_00 & u8x8_broadcast(0x80);

  // Compute the byte index of the first match.
  return stdc_first_trailing_one(hi_set_if_00) >> 3;
}

/**
 * Returns whether an entry in `vec` has a value of `value`.
 */
static inline bool u8x8_contains_match(u8x8 vec, u8 value) {
  return u8x8_contains_zero(vec ^ u8x8_broadcast(value));
}

/**
 * Returns one plus the index of the first entry in `vec` with a value of
 * `value`, or zero if no entry matched.
 */
static inline usize u8x8_find_first_match(u8x8 vec, u8 value) {
  return u8x8_find_first_zero(vec ^ u8x8_broadcast(value));
}

#endif // UKO_OS_KERNEL__SWAR_H
