/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H
#define UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H 1

#include <types.h>

/**
 * Adds a chunk of memory to the physical allocator.
 */
void mm_init_add_physical_chunk(paddr start, paddr end);

/**
 * Allocates a single page of memory from the physical allocator, storing its
 * physical address in `*out`.
 *
 * Returns whether it succeeded.
 */
bool mm_alloc_physical(paddr *out);

#endif // UKO_OS_KERNEL__MM_PHYSICAL_ALLOC_H
