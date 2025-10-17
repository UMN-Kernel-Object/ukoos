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
 * The VMA allocator used for higher-half memory.
 */
extern struct vma_allocator kernel_virtual_allocator;

/**
 * Attempts to initialize a VMA allocator to cover the given address range.
 * Returns whether it succeded.
 */
bool vma_allocator_init(struct vma_allocator *allocator, uaddr lo, uaddr hi);

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
 * Attempts to allocate an contiguous address range of `num_pages` pages.
 * Returns a used VMA of that size on success, or nullptr on failure.
 *
 * Note that this does not (currently) create page table entries.
 */
struct vma *vma_alloc(struct vma_allocator *allocator, usize num_pages);

/**
 * Attempts to allocate the address range `lo` to `hi`. Returns a used VMA
 * that covers exactly that range on success, or nullptr on failure.
 *
 * Note that this does not (currently) create page table entries.
 *
 * TODO: This should indicate "virtual memory already in use" separately from
 * "OOM when trying to allocate a VMA."
 */
struct vma *vma_alloc_by_addr(struct vma_allocator *allocator, uaddr lo,
                              uaddr hi);

/**
 * Frees a VMA.
 *
 * Note that this does not (currently) remove page table entries.
 */
void vma_free(struct vma *vma);

/**
 * Writes the bounds of a VMA to out_lo and out_hi.
 */
void vma_bounds(const struct vma *vma, uaddr *out_lo, uaddr *out_hi);

#endif // UKO_OS_KERNEL__MM_VIRTUAL_ALLOC_H
