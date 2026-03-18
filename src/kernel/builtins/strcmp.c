/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <types.h>

[[gnu::nonnull(1, 2), gnu::pure]]
int strcmp(const char *s1, const char *s2) {
  u8 c1, c2;
  for (;;) {
    c1 = *s1++;
    c2 = *s2++;

    if (c1 != c2)
      break;
    if (!c1)
      break;
  }
  return c1 - c2;
}
