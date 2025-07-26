/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <init.h>

extern struct initializer _kernel_start_initializers[],
    _kernel_end_initializers[];

static void sort_initializers(struct initializer *ptr, usize len) {
  while (len) {
    i32 min_priority = ptr->priority;
    usize min_index = 0;

    for (usize i = 0; i < len; i++) {
      if (ptr[i].priority < min_priority) {
        min_priority = ptr[i].priority;
        min_index = i;
      }
    }

    if (min_index != 0) {
      struct initializer tmp = *ptr;
      *ptr = ptr[min_index];
      ptr[min_index] = tmp;
    }

    ptr++;
    len--;
  }
}

void run_initializers(void) {
  const usize initializers_len =
      (usize)(&_kernel_end_initializers[0] - &_kernel_start_initializers[0]);
  sort_initializers(_kernel_start_initializers, initializers_len);
  for (usize i = 0; i < initializers_len; i++) {
    struct initializer initializer = _kernel_start_initializers[i];
    initializer.func();
  }
}
