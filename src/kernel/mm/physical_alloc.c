/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <align.h>
#include <devicetree.h>
#include <mm/physical_alloc.h>
#include <physical.h>
#include <print.h>

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

static void add_physical_chunk(paddr start, paddr end) {
  // Align up the start address and align down the end address, to page
  // boundaries.
  start = align_up(start, 12);
  end = align_down(end, 12);

  // If the chunk is now empty, ignore it.
  if (bits_of_paddr(start) >= bits_of_paddr(end))
    return;

  // Otherwise, write a link header to it.
  // TODO: The rest of this should have a lock around it.
  print("Found usable physical memory: {paddr}-{paddr}", start, end);
  struct physical_free_list link = {
      .next = free_list_head,
      .length = paddr_diff(end, start) >> 12,
  };
  copy_to_physical(start, &link, sizeof(struct physical_free_list));
  free_list_head = start;
}

/**
 * A memory reservation.
 */
struct memory_reservation {
  paddr start, end;
};

/**
 * Called for each memory region. Finds the chunks that don't overlap with
 * memory reservations, and calls `add_physical_chunk` on each one.
 */
static void consider_physical_memory(paddr addr, usize size,
                                     struct memory_reservation *reservations,
                                     usize reservations_len) {
  paddr start = addr, end = paddr_offset(addr, size);

  // If the start address is inside a reservation, bump it up until it's not any
  // longer.
bump_up_start:
  for (usize i = 0; i < reservations_len; i++) {
    if (paddr_in_range(start, reservations[i].start, reservations[i].end)) {
      start = reservations[i].end;
      goto bump_up_start;
    }
  }

  // If the range is now empty, stop.
  if (bits_of_paddr(start) >= bits_of_paddr(end))
    return;

  // Find the lowest reservation address that now overlaps with the range.
  paddr top = end;
  for (usize i = 0; i < reservations_len; i++)
    if (paddr_in_range(reservations[i].start, start, end))
      top = reservations[i].start;

  add_physical_chunk(start, top);
  start = top;
  goto bump_up_start;
}

/**
 * Flattens all the memory reservations to an array. Returns whether this
 * succeeded.
 */
static bool reserved_memory_to_array(struct devicetree_node *devicetree,
                                     struct memory_reservation **out_ptr,
                                     usize *out_len) {
  // Find the reserved-memory node.
  struct devicetree_node *reserved_memory =
      devicetree_child(devicetree, "reserved-memory");
  if (!reserved_memory)
    return false;

  // Get the #address-cells and #size-cells.
  u32 address_cells, size_cells;
  if (!devicetree_address_size_cells(reserved_memory, &address_cells,
                                     &size_cells))
    return false;
  usize reg_entry_len = (address_cells + size_cells) * 4;

  // Count the number of reservations.
  *out_len = 0;
  for (struct list_head *child = reserved_memory->children.next;
       child != &reserved_memory->children; child = child->next) {
    struct devicetree_node *node =
        container_of(child, struct devicetree_node, parent_children_head);
    struct devicetree_prop *reg = devicetree_prop(node, "reg");
    if (!reg) {
      print("Missing required property reg on a /reserved-memory node");
      continue;
    }

    *out_len += reg->value_len / reg_entry_len;
  }

  // Allocate the array.
  *out_ptr = alloc(*out_len * sizeof(struct memory_reservation));
  if (!*out_ptr)
    return false;

  // Store the reservations into the array.
  usize i = 0;
  for (struct list_head *child = reserved_memory->children.next;
       child != &reserved_memory->children; child = child->next) {
    struct devicetree_node *node =
        container_of(child, struct devicetree_node, parent_children_head);
    struct devicetree_prop *reg = devicetree_prop(node, "reg");
    if (!reg) {
      print("Missing required property reg on a /reserved-memory node");
      continue;
    }

    for (usize j = 0;; j++) {
      paddr addr;
      usize size;
      if (!devicetree_prop_reg(reg, j++, &addr, &size))
        break;

      assert(i < *out_len);
      (*out_ptr)[i++] = (struct memory_reservation){
          .start = addr,
          .end = paddr_offset(addr, size),
      };
    }
  }
  assert(i == *out_len);

  return true;
}

void mm_init_physical(struct devicetree_node *devicetree) {
  struct memory_reservation *reservations;
  usize reservations_len;
  if (!reserved_memory_to_array(devicetree, &reservations, &reservations_len))
    panic("Failed to find memory reservations");
  print("Found {usize} memory reservations:", reservations_len);
  for (usize i = 0; i < reservations_len; i++)
    print("  {paddr}-{paddr}", reservations[i].start, reservations[i].end);

  // Loop over the children, looking for ones whose names start with memory@.
  for (struct list_head *child = devicetree->children.next;
       child != &devicetree->children; child = child->next) {
    struct devicetree_node *node =
        container_of(child, struct devicetree_node, parent_children_head);
    if (strlen(node->name) < 7 || memcmp(node->name, "memory@", 7))
      continue;

    // Run consider_physical_memory() on each one's reg property.
    struct devicetree_prop *reg = devicetree_prop(node, "reg");
    if (!reg) {
      print("Missing required property reg on a memory node");
      continue;
    }

    for (usize i = 0;; i++) {
      paddr addr;
      usize size;
      if (!devicetree_prop_reg(reg, i++, &addr, &size))
        break;

      consider_physical_memory(addr, size, reservations, reservations_len);
    }
  }

  free(reservations);
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
    // There were multiple pages in the link. We'll use the last one, so we
    // can write back the link.
    copy_to_physical(free_list_head, &link, sizeof(struct physical_free_list));
  } else {
    // This is the only page in the link, so update the head pointer to point
    // at the next link.
    free_list_head = link.next;
  }
  *out = paddr_offset(head, link.length << 12);
  return true;
}
