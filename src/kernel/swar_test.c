/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <panic.h>
#include <selftest.h>
#include <swar.h>

DEFINE_SELFTEST() {
  assert(u8x8_broadcast(0x12) == 0x12'12'12'12'12'12'12'12);

  assert(u8x8_find_first_zero(0x00'00'00'00'00'00'00'00) == 1);
  assert(u8x8_find_first_zero(0x00'34'56'78'90'ab'cd'00) == 1);
  assert(u8x8_find_first_zero(0x00'34'56'78'90'ab'cd'ef) == 8);
  assert(u8x8_find_first_zero(0x12'00'56'78'00'ab'cd'ef) == 4);
  assert(u8x8_find_first_zero(0x12'34'56'78'00'ab'cd'ef) == 4);
  assert(u8x8_find_first_zero(0x12'34'56'78'90'ab'cd'00) == 1);
  assert(u8x8_find_first_zero(0x12'34'56'78'90'ab'cd'ef) == 0);
  assert(u8x8_find_first_zero(0x7f'7f'7f'7f'7f'7f'7f'7f) == 0);
  assert(u8x8_find_first_zero(0x80'80'80'80'80'80'80'80) == 0);
  assert(u8x8_find_first_zero(0x81'81'81'81'81'81'81'81) == 0);
  assert(u8x8_find_first_zero(0xff'ff'ff'ff'ff'ff'ff'ff) == 0);
}
