/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "block.h"
#include "page.h"
#include <align.h>

struct mm_alloc_block_ref block_ref_make(struct mm_alloc_page *page,
                                         struct mm_alloc_block *block) {
  assert((page->xor_cookie & 0b111) == 0b111);

  uaddr block_addr = addr_of_ptr(block);

  if (block) {
    assert(page_in_bounds(page, block_addr));
    assert(is_aligned(block, 3));
    assert((block_addr >> 56) == 0xff);
  }

  struct mm_alloc_block_ref out = {
      .xorred_ptr = block_addr ^ page->xor_cookie,
  };
  assert(block_ref_is_valid(out));
  return out;
}

struct mm_alloc_block *block_ref_deref(const struct mm_alloc_page *page,
                                       struct mm_alloc_block_ref ref) {
  assert((page->xor_cookie & 0b111) == 0b111);
  assert(block_ref_is_valid(ref));
  uaddr block_addr = ref.xorred_ptr ^ page->xor_cookie;
  if (block_addr) {
    assert(page_in_bounds(page, block_addr));
    assert((block_addr >> 56) == 0xff);
  }
  return ptr_with_addr((void *)page, block_addr);
}

bool block_ref_is_valid(struct mm_alloc_block_ref ref) {
  return (ref.xorred_ptr & 0b111) == 0b111;
}
