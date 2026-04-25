/*
 * SPDX-FileCopyrightText: ukoOS Contributors
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
 * Like `alloc`, but only for allocations with `0 < size && size <= 1024`.
 */
[[gnu::alloc_size(1), gnu::malloc, gnu::malloc(free, 1), nodiscard]] void *
alloc_small(usize size, struct mm_alloc_heap *heap);

/**
 * The slow path of `alloc`.
 */
[[gnu::alloc_size(1), gnu::malloc, gnu::malloc(free, 1), nodiscard]] void *
alloc_generic(usize size, struct mm_alloc_heap *heap);

/**
 * Allocates `size` bytes of memory and returns a pointer to it. On OOM, returns
 * `nullptr`.
 *
 * - if `size` is greater than or equal to 1 and less than or equal to 64, the
 *   returned pointer will be aligned to at least `stdc_bit_ceil(size)`.
 * - if `size` is greater than 64, the returned pointer will be aligned to at
 *   least 64.
 */
[[gnu::alloc_size(1), gnu::malloc, nodiscard]] static inline void *
alloc(usize size) {
  struct mm_alloc_heap *heap = get_hart_locals()->heap;
  // Bump up the size if it's zero, since zero isn't a valid input to
  // alloc_small.
  //
  // Bitwise math in pages_direct_index_of_size wouldn't be correct with a zero
  // input, and we don't want to put a branch for it in the fast-path. This
  // function is expected to be inlined, and the size argument should be
  // constant (or at least provably non-zero) in the caller most of the time, so
  // the branch should be eliminated.
  if (size == 0)
    return alloc_small(1, heap);
  else if (size <= 1024)
    return alloc_small(size, heap);
  else
    return alloc_generic(size, heap);
}

/**
 * Allocates zeroed-out memory. See `alloc` for the other conditions of this
 * function.
 */

[[gnu::alloc_size(1), gnu::malloc, nodiscard]] static inline void *
zalloc(usize size) {
  void *out = alloc(size);
  if (out)
    bzero(out, size);
  return out;
}

/**
 * Allocates a copy of an object.
 */
[[gnu::alloc_size(2), gnu::malloc, nodiscard]] static inline void *
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
[[gnu::malloc, nodiscard]] static inline char *strdup(const char *str) {
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

/**
 * Sets up the initial heap.
 */
void mm_alloc_init(void);

#endif // UKO_OS_KERNEL__MM_ALLOC_H
