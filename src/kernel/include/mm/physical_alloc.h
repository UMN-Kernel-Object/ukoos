/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H
#define UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H 1

#include <devicetree.h>

/**
 * The possible flags to pass to the allocator
 */
enum paging_flags {
  PAGING_DEFAULT,
  PAGING_BELOW_4G,
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
bool mm_alloc_physical(paddr *out, enum paging_flags f);

/**
 * Frees a single frame of memory.
 */
void mm_free_physical(paddr frame);

#endif // UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H
