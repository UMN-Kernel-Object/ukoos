/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <types.h>

ATTR_ACCESS(write_only, 1)
[[gnu::nonnull(1)]]
void explicit_bzero(void *dst, usize len) {
  bzero(dst, len);
  __asm__ volatile("" ::"r"(dst) : "memory");
}
