/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_VIRTUAL_ALLOC_H
#define UKO_OS_KERNEL__MM_VIRTUAL_ALLOC_H 1

#include <list.h>

/**
 * A virtual memory area. This struct represents a region of either free or
 * allocated memory, with page granularity or greater.
 */
struct vma;

/**
 * An instance of the virtual memory allocator.
 */
struct vma_allocator;

/**
 * Creates a new instance of the virtual memory allocator, set to cover the
 * given address range.
 */
struct vma_allocator *vma_allocator_new(uaddr lo, uaddr hi);

/**
 * Prints the VMA allocator's state, for debugging purposes.
 */
void vma_allocator_print(struct vma_allocator *allocator);

/**
 * Allocates a range of virtual address space from the given allocator.
 *
 * `size` must not be zero.
 */
[[gnu::nonnull(1)]] uptr mm_va_alloc(struct virtual_buddy *allocator,
                                     usize size);

/**
 * Frees a range of virtual address space allocated with `mm_va_alloc`.
 *
 * `size` must be the size that was originally allocated.
 */
[[gnu::nonnull(1)]] void mm_va_free(struct virtual_buddy *allocator, uptr ptr,
                                    usize size);

#endif // UKO_OS_KERNEL__MM_VIRTUAL_ALLOC_H
