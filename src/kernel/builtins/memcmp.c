/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <types.h>

[[gnu::nonnull(1, 2), gnu::pure]]
int memcmp(const void *s1, const void *s2, usize n) {
  const u8 *p1 = s1, *p2 = s2;
  while (n--) {
    if (*p1 < *p2)
      return -1;
    if (*p1 > *p2)
      return 1;
    p1++;
    p2++;
  }
  return 0;
}
