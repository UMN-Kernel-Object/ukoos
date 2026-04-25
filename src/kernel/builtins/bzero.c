/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <types.h>

ATTR_ACCESS(write_only, 1)
[[gnu::nonnull(1)]]
void bzero(void *dst, usize len) {
  u8 *p = dst;
  while (len--)
    *p++ = 0;
}
