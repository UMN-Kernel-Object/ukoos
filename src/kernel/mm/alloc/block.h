/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_ALLOC_BLOCK_H
#define UKO_OS_KERNEL__MM_ALLOC_BLOCK_H 1

#include <types.h>

struct mm_alloc_page;

/**
 * The address of a block, XORred by the `xor_cookie` from the block's page.
 */
struct mm_alloc_block_ref {
  uaddr xorred_ptr;
};

/**
 * Each page has several free lists composed of "blocks"
 * (`struct mm_alloc_block`). Blocks are just links in a linked list.
 */
struct mm_alloc_block {
  /**
   * The next block.
   */
  struct mm_alloc_block_ref next;
};

/**
 * Creates a block reference.
 */
struct mm_alloc_block_ref block_ref_make(struct mm_alloc_page *page,
                                         struct mm_alloc_block *block);

/**
 * Dereferences a block reference.
 */
struct mm_alloc_block *block_ref_deref(const struct mm_alloc_page *page,
                                       struct mm_alloc_block_ref ref);

/**
 * Returns whether a block reference is valid.
 */
bool block_ref_is_valid(struct mm_alloc_block_ref ref);

#endif // UKO_OS_KERNEL__MM_ALLOC_BLOCK_H
