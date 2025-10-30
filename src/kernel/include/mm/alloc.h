/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_ALLOC_H
#define UKO_OS_KERNEL__MM_ALLOC_H 1

#include <hart_locals.h>

/**
 * Frees memory returned by a previous call to `alloc`.
 *
 * When passed `nullptr`, does nothing. This makes code using
 * `[[gnu::cleanup]]` easier to write.
 */
void free(void *ptr);

/**
 * Allocates `size` bytes of memory and returns a pointer to it. On OOM, returns
 * `nullptr`.
 *
 * - if `size` is greater than equal to 1 and less than or equal to 64, the
 *   returned pointer will be aligned to at least `stdc_bit_ceil(size)`.
 * - if `size` is greater than 64, the returned pointer will be aligned to at
 *   least 64.
 */
[[gnu::alloc_size(1), gnu::malloc, gnu::malloc(free, 1), nodiscard]] void *
alloc(usize size);

/**
 * Allocates zeroed-out memory. See `alloc` for the other conditions of this
 * function.
 */
[[gnu::alloc_size(1), gnu::malloc, gnu::malloc(free, 1), nodiscard]] void *
zalloc(usize size);

/**
 * Allocates a copy of an object.
 */
[[gnu::alloc_size(2), gnu::malloc, gnu::malloc(free, 1),
  nodiscard]] static void *
memdup(const void *ptr, usize len) {
  char *out = alloc(len);
  if (!out)
    return nullptr;
  return memcpy(out, ptr, len);
}

/**
 * Allocates a string, which is the same length as `str` and initialized to have
 * the same contents.
 */
[[gnu::malloc, gnu::malloc(free, 1), nodiscard]] static char *
strdup(const char *str) {
  return memdup(str, strlen(str) + 1);
}

/**
 * Allocates memory, initializing it with the contents of `ptr`, which is then
 * `free`d. On OOM, returns `nullptr` and does *not* free `ptr`.
 *
 * - if `new_size` is greater than equal to 1 and less than or equal to 64, the
 *   returned pointer will be aligned to at least `stdc_bit_ceil(new_size)`.
 * - if `new_size` is greater than 64, the returned pointer will be aligned to
 *   at least 64.
 */
[[gnu::alloc_size(2), nodiscard]] void *realloc(void *ptr, usize new_size);

#endif // UKO_OS_KERNEL__MM_ALLOC_H
