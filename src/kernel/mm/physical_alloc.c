/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <align.h>
#include <mm/physical_alloc.h>
#include <physical.h>

/**
 * The physical memory allocator. This is a simple free list allocator.
 */

struct physical_free_list {
  paddr next;
  // The length of the free list, including this link.
  usize length;
};

/**
 * The head of the free list.
 */
static paddr free_list_head = {0};

void mm_init_add_physical_chunk(paddr start, paddr end) {
  // Align up the start address and align down the end address, to page
  // boundaries.
  start = align_up(start, 12);
  end = align_down(end, 12);

  // If the chunk is now empty, ignore it.
  if (bits_of_paddr(start) >= bits_of_paddr(end))
    return;

  // Otherwise, write a link header to it.
  // TODO: The rest of this should have a lock around it.
  struct physical_free_list link = {
      .next = free_list_head,
      .length = paddr_diff(end, start) >> 12,
  };
  copy_to_physical(start, &link, sizeof(struct physical_free_list));
  free_list_head = start;
}

bool mm_alloc_physical(paddr *out) {
  // TODO: There should be a lock around essentially this whole function.
  paddr head = free_list_head;
  if (!bits_of_paddr(head))
    return false;

  struct physical_free_list link;
  copy_from_physical(&link, head, sizeof(struct physical_free_list));
  assert(link.length > 0);
  link.length--;
  if (link.length) {
    // There were multiple pages in the link. We'll use the last one, so we can
    // write back the link.
    copy_to_physical(free_list_head, &link, sizeof(struct physical_free_list));
  } else {
    // This is the only page in the link, so update the head pointer to point at
    // the next link.
    free_list_head = link.next;
  }
  *out = paddr_offset(head, link.length);
  return true;
}
