/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H
#define UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H 1

#include <devicetree.h>
#include <mm/paging.h>

/**
 * The possible flags to pass to the allocator
 */
enum physical_alloc_flags {
  PHYSICAL_ALLOC_DEFAULT = 0,
  PHYSICAL_ALLOC_BELOW_4G = (1 << 0),
};

/**
 * Initializes the physical allocator with the memory discovered in the
 * Devicetree, ignoring memory reservations.
 */
void mm_init_physical(struct devicetree_node *devicetree);

/**
 * Allocates a single frame of memory from the physical allocator, storing its
 * physical address in `*out`.
 *
 * Returns whether it succeeded.
 */
[[gnu::warn_unused_result]]
bool mm_alloc_physical(paddr *out, enum physical_alloc_flags flags);

/**
 * Frees a single frame of memory.
 */
void mm_free_physical(paddr frame);

/**
 * Allocates and maps pages to fill the range [lo, hi), which must be
 * page-aligned. Note that this does not call mm_paging_fence.
 *
 * Returns whether it succeeded.
 */
[[gnu::warn_unused_result]]
bool mm_alloc_and_map_many(uaddr lo, uaddr hi, enum physical_alloc_flags flags,
                           enum page_permissions perms);

#endif // UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H
