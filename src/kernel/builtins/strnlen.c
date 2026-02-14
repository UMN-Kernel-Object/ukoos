/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <types.h>

[[gnu::nonnull(1), gnu::pure]] usize strnlen(const char *s, usize max_len) {
  usize n = 0;
  while (max_len-- && *s++)
    n++;
  return n;
}
