/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <align.h>
#include <arch/riscv64/constants.h>
#include <devicetree.h>
#include <mm/physical_alloc.h>
#include <physical.h>
#include <print.h>

/**
 * The physical memory allocator. This is a simple free list allocator.
 */

struct physical_free_list {
  paddr next;
  // The length of the chunk in pages, including the page containing this link.
  usize length;
};

/**
 * The head of the free list.
 */
static paddr free_list_head_above_4g = {0};
static paddr free_list_head_below_4g = {0};

static void physical_free_list_push(usize length, paddr start,
                                    paddr *free_list_head) {
  struct physical_free_list link = {
      .next = *free_list_head,
      .length = length,
  };
  copy_to_physical(start, &link, sizeof(struct physical_free_list));
  *free_list_head = start;
}

static bool physical_free_list_pop(paddr *out, paddr *free_list_head) {
  *out = *free_list_head;
  if (!bits_of_paddr(*out))
    return false;

  struct physical_free_list link;
  copy_from_physical(&link, *free_list_head, sizeof(struct physical_free_list));
  assert(link.length > 0);
  if (link.length == 1) {
    // This is the only page in the link, so update the head pointer to point
    // at the next link.
    *free_list_head = link.next;
  } else {
    // There were multiple pages in the link. We'll use the first one, so we
    // write back the link to the next one.
    *free_list_head = paddr_offset(*free_list_head, PAGE_SIZE);
    link.length--;
    copy_to_physical(*free_list_head, &link, sizeof(struct physical_free_list));
  }
  return true;
}

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
  if (bits_of_paddr(start) >= (1ul << 32)) {
    physical_free_list_push(paddr_diff(end, start) >> 12, start,
                            &free_list_head_above_4g);
  } else if (bits_of_paddr(start) < (1ul << 32) &&
             bits_of_paddr(end) >= (1ul << 32)) {
    paddr old_end = end;
    end = paddr_of_bits(1ul << 32);
    physical_free_list_push(paddr_diff(end, start) >> 12, start,
                            &free_list_head_below_4g);
    start = end;
    end = old_end;
    physical_free_list_push(paddr_diff(end, start) >> 12, start,
                            &free_list_head_above_4g);
  } else {
    physical_free_list_push(paddr_diff(end, start) >> 12, start,
                            &free_list_head_below_4g);
  }
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
      devicetree_print(node);
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
    if (!reg)
      continue;

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
    if (strlen(node->name) < sizeof("memory@") - 1 ||
        memcmp(node->name, "memory@", sizeof("memory@") - 1))
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

bool mm_alloc_physical(paddr *out, enum physical_alloc_flags flags) {
  // TODO: There should be a lock around essentially this whole function.

  if (!(flags & PHYSICAL_ALLOC_BELOW_4G) &&
      physical_free_list_pop(out, &free_list_head_above_4g))
    return true;

  return physical_free_list_pop(out, &free_list_head_below_4g);
}

void mm_free_physical(paddr frame) {
  // TODO: There should be a lock around essentially this whole function.
  assert(bits_of_paddr(frame));

  if (bits_of_paddr(frame) >= (1ul << 32)) {
    physical_free_list_push(1, frame, &free_list_head_above_4g);
  } else {
    physical_free_list_push(1, frame, &free_list_head_below_4g);
  }
}

bool mm_alloc_and_map_many(uaddr lo, uaddr hi, enum physical_alloc_flags flags,
                           enum page_permissions perms) {
  assert(is_aligned(lo, PAGE_BITS));
  assert(is_aligned(hi, PAGE_BITS));

  uaddr va = lo;
  paddr pa;
  while (va < hi) {
    // Invariant: va is not mapped.
    if (!mm_alloc_physical(&pa, flags))
      goto fail;
    if (!mm_paging_map(va, pa, perms)) {
      mm_free_physical(pa);
      goto fail;
    }
    // Invariant: va is mapped.
    va += PAGE_SIZE;
  }
  return true;

fail:
  while (va > lo) {
    // Invariant: va is not mapped.
    va -= PAGE_SIZE;
    assert(mm_paging_unmap(va, &pa));
    mm_free_physical(pa);
  }
  assert(va == lo);
  // Invariant: lo is not mapped.
  return false;
}
