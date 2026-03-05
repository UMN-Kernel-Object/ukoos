/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <types.h>

[[gnu::nonnull(1), gnu::pure]] usize strlen(const char *s) {
  usize n = 0;
  while (*s++)
    n++;
  return n;
}
