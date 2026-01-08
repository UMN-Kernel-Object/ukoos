/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_ALLOC_SIZE_CLASSES_H
#define UKO_OS_KERNEL__MM_ALLOC_SIZE_CLASSES_H 1

#include <panic.h>

/* ## Object sizes
 *
 * Objects can be "small," "large," or "huge." This is determined by the size:
 *
 * - small:          size <= 8KiB
 * - large: 8KiB   < size <= 512KiB
 * - huge:  512KiB < size
 */

static inline bool size_is_small(usize size) { return size <= 8 * 1024; }
static inline bool size_is_huge(usize size) { return 512 * 1024 < size; }

/* Small objects get a special fast path in the allocator. The pages that
 * contain them are 64KiB, and segments that contain small-object pages contain
 * 64 pages (and so are 4MiB in size).
 */

/**
 * The number of bits to shift the difference between a heap address and the
 * address of its segment by in order to get the index of the page it belongs
 * to, when the page contains small objects.
 */
static constexpr usize PAGE_SMALL_SHIFT = 16;

/* Pages that contain large objects are 4MiB in size, and segments that contain
 * them only contain one page (and so are 4MiB in size).
 */

/**
 * The number of bits to shift the difference between a heap address and the
 * address of its segment by in order to get the index of the page it belongs
 * to, when the page contains large objects.
 */
static constexpr usize PAGE_LARGE_SHIFT = 22;

/* Pages that contain huge objects only contain a single object. They can be
 * larger or smaller than 4MiB, and their segments are accordingly sized.
 */

/**
 * The number of bits to shift the difference between a heap address and the
 * address of its segment by in order to get the index of the page it belongs
 * to, when the page contains huge objects.
 *
 * This is smaller than the paper says; I believe the value in the paper (22) to
 * be an error. Huge pages only have one allocation, so their page shift is
 * smaller than the other pages. (This also lets us distinguish huge pages by
 * their shift.)
 */
static constexpr usize PAGE_HUGE_SHIFT = 14;

/**
 * The log2 of the size of a segment that contains pages for small or large
 * objects.
 */
static constexpr usize SEGMENT_SHIFT = 22;
static_assert((1 << SEGMENT_SHIFT) == (4 * 1024 * 1024));

/* Each segment is composed of one or more "pages." These should not be confused
 * with hardware pages; while hardware pages are typically 4KiB, allocator pages
 * are at least 64KiB. The segment stores all the metadata for pages; the first
 * page is shortened accordingly.
 */

/**
 * Returns whether a size class is valid for a small or large object.
 */
static inline bool size_class_is_valid(usize size_class) {
  return size_class < 17;
}

/**
 * A sentinel for huge pages. Because these are only ever in a state where
 * they're full (since they only contain one object, and don't get pooled when
 * empty), this "size class" is _not_ treated as one by `size_class_is_valid`.
 */
static constexpr usize SIZE_CLASS_HUGE_SENTINEL = 17;

/**
 * Returns whether a size class is valid for a small object.
 */
static inline bool size_class_is_small(usize size_class) {
  assert(size_class_is_valid(size_class));
  return size_class < 11;
}

/**
 * Given a size, returns the size class of an allocation of that size.
 */
static inline usize size_class_of_size(usize size) {
  assert(!size_is_huge(size));
  if (size < 8)
    return 0;
  usize log2_size = stdc_trailing_zeros(stdc_bit_ceil(size));
  assert(log2_size >= 3);
  usize size_class = log2_size - 3;
  assert(size_class_is_valid(size_class));
  return size_class;
}

/**
 * Given a size class, returns the log2 of the size of the allocation of that
 * size class.
 */
static inline usize log2_size_of_size_class(usize size_class) {
  assert(size_class_is_valid(size_class));
  return 3 + size_class;
}

/**
 * Given a size class, returns the size of the allocation of that size class.
 */
static inline usize size_of_size_class(usize size_class) {
  return (usize)1 << log2_size_of_size_class(size_class);
}

#endif // UKO_OS_KERNEL__MM_ALLOC_SIZE_CLASSES_H
