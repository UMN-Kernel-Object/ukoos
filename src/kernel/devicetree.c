/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <align.h>
#include <devicetree.h>
#include <endian.h>
#include <mm/alloc.h>
#include <print.h>
#include <random.h>

struct fdt_header {
  u32 magic;
  u32 totalsize;
  u32 off_dt_struct;
  u32 off_dt_strings;
  u32 off_mem_rsvmap;
  u32 version;
  u32 last_comp_version;
  u32 boot_cpuid_phys;
  u32 size_dt_strings;
  u32 size_dt_struct;
};

struct fdt_reserve_entry {
  u64 address;
  u64 size;
};

enum fdt_struct_token_type : u32 {
  FDT_BEGIN_NODE = 1,
  FDT_END_NODE = 2,
  FDT_PROP = 3,
  FDT_NOP = 4,
  FDT_END = 9,
};

static struct fdt_header physical_read_fdt_header(paddr paddr) {
  return (struct fdt_header){
      .magic = physical_read_u32be(paddr_offset(paddr, 0)),
      .totalsize = physical_read_u32be(paddr_offset(paddr, 4)),
      .off_dt_struct = physical_read_u32be(paddr_offset(paddr, 8)),
      .off_dt_strings = physical_read_u32be(paddr_offset(paddr, 12)),
      .off_mem_rsvmap = physical_read_u32be(paddr_offset(paddr, 16)),
      .version = physical_read_u32be(paddr_offset(paddr, 20)),
      .last_comp_version = physical_read_u32be(paddr_offset(paddr, 24)),
      .boot_cpuid_phys = physical_read_u32be(paddr_offset(paddr, 28)),
      .size_dt_strings = physical_read_u32be(paddr_offset(paddr, 32)),
      .size_dt_struct = physical_read_u32be(paddr_offset(paddr, 36)),
  };
}

static struct fdt_reserve_entry physical_read_fdt_reserve_entry(paddr paddr) {
  return (struct fdt_reserve_entry){
      .address = physical_read_u64be(paddr_offset(paddr, 0)),
      .size = physical_read_u64be(paddr_offset(paddr, 8)),
  };
}

static struct devicetree_node *
devicetree_parse_structure_block(paddr devicetree_start,
                                 const struct fdt_header *header) {
  struct devicetree_node *current_node = nullptr;

  paddr here = paddr_offset(devicetree_start, header->off_dt_struct);
  paddr struct_end = paddr_offset(here, header->size_dt_struct);
  bool finished_root_node = false, got_end_token = false;
  while (bits_of_paddr(here) != bits_of_paddr(struct_end)) {
    enum fdt_struct_token_type token_type = physical_read_u32be(here);
    here = paddr_offset(here, 4);
    switch (token_type) {
    case FDT_BEGIN_NODE: {
      if (finished_root_node) {
        print("{cstr}: got FDT_BEGIN_NODE token after the root node?",
              __func__);
        goto fail;
      }

      // Compute the length of the name.
      usize name_len = 0;
      while (physical_read_u8(paddr_offset(here, name_len)))
        name_len++;

      // Construct the new node, making the current node its parent.
      struct devicetree_node *node = alloc(sizeof(struct devicetree_node));
      if (!node)
        goto fail;
      *node = (struct devicetree_node){
          .parent = current_node,
          .parent_children_head = LIST_INIT(node->parent_children_head),
          .children = LIST_INIT(node->children),
          .props = LIST_INIT(node->props),
          .name = alloc(name_len + 1),
      };
      if (!node->name) {
        free(node);
        goto fail;
      }
      if (current_node)
        list_push(&current_node->children, &node->parent_children_head);

      // Copy the name into the new node and make the new node the current node.
      copy_from_physical(node->name, here, name_len);
      node->name[name_len] = '\0';
      current_node = node;

      // Advance to the next token.
      here = align_up(paddr_offset(here, name_len + 1), 2);
    } break;

    case FDT_END_NODE: {
      if (!current_node || finished_root_node) {
        print("{cstr}: got FDT_END_NODE token outside of a node?", __func__);
        goto fail;
      }

      // If we're at the root node, save the node. Otherwise, traverse back to
      // the parent.
      if (current_node->parent)
        current_node = current_node->parent;
      else
        finished_root_node = true;
    } break;

    case FDT_PROP: {
      if (!current_node || finished_root_node) {
        print("{cstr}: got FDT_PROP token outside of a node?", __func__);
        goto fail;
      }

      // Read the fixed fields.
      u32 value_len = physical_read_u32be(here);
      here = paddr_offset(here, 4);
      u32 name_off = physical_read_u32be(here);
      here = paddr_offset(here, 4);

      // Find the bounds of the name.
      paddr name_start =
          paddr_offset(devicetree_start, header->off_dt_strings + name_off);
      usize name_len = 0;
      while (physical_read_u8(paddr_offset(name_start, name_len)))
        name_len++;

      // Skip the value.
      paddr value_start = here;
      here = align_up(paddr_offset(here, value_len), 2);

      // Allocate the property.
      struct devicetree_prop *prop = alloc(sizeof(struct devicetree_prop));
      if (!prop)
        goto fail;
      *prop = (struct devicetree_prop){
          .node = current_node,
          .node_props_head = LIST_INIT(prop->node_props_head),
          .name = alloc(name_len + 1),
          .value_len = value_len,
          .value = alloc(value_len),
      };
      if (!prop->name || !prop->value) {
        free(prop->name);
        free(prop->value);
        free(prop);
        goto fail;
      }

      // Fill out the name and value, and link the prop into the current node.
      copy_from_physical(prop->name, name_start, name_len + 1);
      copy_from_physical(prop->value, value_start, value_len);
      list_push(&current_node->props, &prop->node_props_head);
    } break;

    case FDT_NOP: {
      // Just skip the token.
    } break;

    case FDT_END: {
      // Check that we're in the right place to stop.
      if (bits_of_paddr(here) != bits_of_paddr(struct_end)) {
        print("{cstr}: got FDT_END token not at the end?", __func__);
        goto fail;
      }
      got_end_token = true;
    } break;

    default:
      TODO("unknown token type {u32}", token_type);
    }
  }

  if (!got_end_token) {
    print("{cstr}: missing FDT_END token?", __func__);
    goto fail;
  }
  if (!current_node) {
    print("{cstr}: missing root node?", __func__);
    goto fail;
  }
  if (!finished_root_node) {
    print("{cstr}: Devicetree ended inside a node?", __func__);
    goto fail;
  }

  return current_node;

fail:
  devicetree_free(current_node);
  return nullptr;
}

static bool devicetree_add_prop(struct devicetree_node *node, const char *name,
                                const u8 *value, usize value_len) {
  struct devicetree_prop *prop = alloc(sizeof(struct devicetree_prop));
  if (!prop)
    return false;

  *prop = (struct devicetree_prop){
      .node = node,
      .node_props_head = LIST_INIT(prop->node_props_head),
      .name = strdup(name),
      .value_len = value_len,
      .value = memdup(value, value_len),
  };
  if (!prop->name || !prop->value) {
    free(prop->name);
    free(prop->value);
    free(prop);
    return false;
  }

  list_push(&node->props, &prop->node_props_head);
  return true;
}

static bool devicetree_add_prop_u32(struct devicetree_node *node,
                                    const char *name, u32 value) {
  u8 bytes[4];
  value = native_to_big(value);
  memcpy(&bytes, &value, 4);
  return devicetree_add_prop(node, name, bytes, 4);
}

static bool devicetree_add_reservation(struct devicetree_node *reserved_memory,
                                       usize *reservation_num,
                                       struct fdt_reserve_entry reservation,
                                       u32 address_cells, u32 size_cells) {
  // Allocate a node for the reservation.
  struct devicetree_node *node = alloc(sizeof(struct devicetree_node));
  if (!node)
    return false;

  // Choose a name for the reservation.
  char *reservation_name = nullptr;
  do {
    if (reservation_name)
      free(reservation_name);
    reservation_name =
        format("memory-reservation-block@{usize}", (*reservation_num)++);
    if (!reservation_name)
      goto fail;
  } while (devicetree_child(reserved_memory, reservation_name));

  // Initialize the node.
  *node = (struct devicetree_node){
      .parent = reserved_memory,
      .parent_children_head = LIST_INIT(node->parent_children_head),
      .children = LIST_INIT(node->children),
      .props = LIST_INIT(node->props),
      .name = reservation_name,
  };
  list_push(&reserved_memory->children, &node->parent_children_head);

  // Add the reg field to the props.
  u8 reg[16];
  usize reg_len = 0;
  switch (address_cells) {
  case 1: {
    u32 address = (u32)native_to_big(reservation.address);
    memcpy(&reg[reg_len], &address, 4);
    reg_len += 4;
  } break;
  case 2: {
    u64 address = native_to_big(reservation.address);
    memcpy(&reg[reg_len], &address, 8);
    reg_len += 8;
  } break;
  default:
    print("Invalid value for /reserved-memory's #address-cells: {u32}",
          address_cells);
    goto fail;
  }
  switch (size_cells) {
  case 1: {
    u32 size = (u32)native_to_big(reservation.size);
    memcpy(&reg[reg_len], &size, 4);
    reg_len += 4;
  } break;
  case 2: {
    u64 size = native_to_big(reservation.size);
    memcpy(&reg[reg_len], &size, 8);
    reg_len += 8;
  } break;
  default:
    print("Invalid value for /reserved-memory's #size-cells: {u32}",
          size_cells);
    goto fail;
  }

  if (!devicetree_add_prop(node, "reg", reg, reg_len))
    goto fail;

  return true;

fail:
  list_remove(&node->parent_children_head);
  free(node->name);
  free(node);
  return false;
}

/**
 * Adds reservations from the reserved memory block, and adds the kernel itself,
 * to the /reserved-memory node of the Devicetree.
 */
static bool devicetree_add_reservations(paddr devicetree_start,
                                        paddr kernel_start, paddr kernel_end,
                                        struct fdt_header *header,
                                        struct devicetree_node *devicetree) {
  // Ensure the reserved-memory node exists.
  struct devicetree_node *reserved_memory =
      devicetree_child(devicetree, "reserved-memory");
  if (!reserved_memory) {
    // Create the reserved-memory node.
    reserved_memory = alloc(sizeof(struct devicetree_node));
    if (!reserved_memory)
      return false;
    *reserved_memory = (struct devicetree_node){
        .parent = devicetree,
        .parent_children_head =
            LIST_INIT(reserved_memory->parent_children_head),
        .children = LIST_INIT(reserved_memory->children),
        .props = LIST_INIT(reserved_memory->props),
        .name = strdup("reserved-memory"),
    };
    if (!reserved_memory->name) {
      free(reserved_memory);
      return false;
    }
    list_push(&devicetree->children, &reserved_memory->parent_children_head);

    // Add its required props.
    if (!devicetree_add_prop_u32(reserved_memory, "#address-cells", 2))
      return false;
    if (!devicetree_add_prop_u32(reserved_memory, "#size-cells", 2))
      return false;
    if (!devicetree_add_prop(reserved_memory, "ranges", nullptr, 0))
      return false;
  }

  // Get the number of address and size cells.
  struct devicetree_prop *address_cells_prop =
                             devicetree_prop(reserved_memory, "#address-cells"),
                         *size_cells_prop =
                             devicetree_prop(reserved_memory, "#size-cells");
  if (!address_cells_prop) {
    print("Invalid value for /reserved-memory's #address-cells: missing");
    return false;
  }
  if (!size_cells_prop) {
    print("Invalid value for /reserved-memory's #size-cells: missing");
    return false;
  }
  if (address_cells_prop->value_len != 4) {
    print("Invalid value for /reserved-memory's #address-cells: too long");
    return false;
  }
  if (size_cells_prop->value_len != 4) {
    print("Invalid value for /reserved-memory's #size-cells: too long");
    return false;
  }
  u32 address_cells, size_cells;
  memcpy(&address_cells, address_cells_prop->value, 4);
  memcpy(&size_cells, size_cells_prop->value, 4);
  address_cells = big_to_native(address_cells);
  size_cells = big_to_native(size_cells);

  // Adds reservations from the memory reservations block to the reserved-memory
  // node.
  paddr next_reservation =
      paddr_offset(devicetree_start, header->off_mem_rsvmap);
  usize reservation_num = 0;
  struct fdt_reserve_entry reservation = {0};
  for (;;) {
    reservation = physical_read_fdt_reserve_entry(next_reservation);
    next_reservation =
        paddr_offset(next_reservation, sizeof(struct fdt_reserve_entry));
    if (reservation.address == 0 && reservation.size == 0)
      break;

    if (!devicetree_add_reservation(reserved_memory, &reservation_num,
                                    reservation, address_cells, size_cells))
      return false;
  }

  // Add a reservation for the kernel.
  reservation.address = bits_of_paddr(kernel_start);
  reservation.size = paddr_diff(kernel_end, kernel_start);
  if (!devicetree_add_reservation(reserved_memory, &reservation_num,
                                  reservation, address_cells, size_cells))
    return false;

  return true;
}

struct devicetree_node *devicetree_parse_from_physical(paddr devicetree_start,
                                                       paddr kernel_start,
                                                       paddr kernel_end) {
  struct devicetree_node *devicetree = nullptr;

  // Parse the header.
  struct fdt_header header = physical_read_fdt_header(devicetree_start);
  if (header.magic != 0xd00dfeed) {
    print("Devicetree magic value did not match (got {u32:#010x}, expected "
          "0xd00dfeed)",
          header.magic);
    goto fail;
  }
  if (header.last_comp_version > 17) {
    print("Devicetree was not compatible with our parser");
    goto fail;
  }

  // Parse the actual tree.
  devicetree = devicetree_parse_structure_block(devicetree_start, &header);
  if (!devicetree)
    goto fail;

  // Parse the memory reservations block and add it to the tree.
  if (!devicetree_add_reservations(devicetree_start, kernel_start, kernel_end,
                                   &header, devicetree))
    goto fail;

  return devicetree;

fail:
  devicetree_free(devicetree);
  return nullptr;
}

static void devicetree_prop_free(struct devicetree_prop *prop) {
  list_remove(&prop->node_props_head);
  free(prop->name);
  free(prop->value);
  free(prop);
}

void devicetree_free(struct devicetree_node *node) {
  while (node) {
    // Free every property.
    while (!list_is_empty(&node->props))
      devicetree_prop_free(container_of(
          node->props.next, struct devicetree_prop, node_props_head));

    // If we're childless, free ourselves and move up to our parent.
    while (list_is_empty(&node->children)) {
      assert(list_is_empty(&node->props));
      struct devicetree_node *old_node = node;
      node = old_node->parent;
      if (node)
        list_remove(&old_node->parent_children_head);
      free(old_node->name);
      free(old_node);
      if (!node)
        return;
    }
    assert(node);
    assert(list_is_empty(&node->props));
    assert(!list_is_empty(&node->children));

    // Otherwise, descend into our first child.
    node = container_of(node->children.next, struct devicetree_node,
                        parent_children_head);
  }
}

struct devicetree_node *devicetree_child(struct devicetree_node *parent,
                                         const char *name) {
  for (struct list_head *child = parent->children.next;
       child != &parent->children; child = child->next) {
    struct devicetree_node *node =
        container_of(child, struct devicetree_node, parent_children_head);
    if (!strcmp(node->name, name))
      return node;
  }
  return nullptr;
}

struct devicetree_prop *devicetree_prop(struct devicetree_node *parent,
                                        const char *name) {
  for (struct list_head *child = parent->props.next; child != &parent->props;
       child = child->next) {
    struct devicetree_prop *prop =
        container_of(child, struct devicetree_prop, node_props_head);
    if (!strcmp(prop->name, name))
      return prop;
  }
  return nullptr;
}

bool devicetree_address_size_cells(struct devicetree_node *node,
                                   u32 *out_address_cells,
                                   u32 *out_size_cells) {
  struct devicetree_prop *address_cells_prop =
                             devicetree_prop(node, "#address-cells"),
                         *size_cells_prop =
                             devicetree_prop(node, "#size-cells");
  if (!address_cells_prop) {
    print("Missing value for #address-cells");
    return false;
  }
  if (!size_cells_prop) {
    print("Missing value for #size-cells");
    return false;
  }
  if (address_cells_prop->value_len != 4) {
    print("Invalid value for #address-cells: too long");
    return false;
  }
  if (size_cells_prop->value_len != 4) {
    print("Invalid value for #size-cells: too long");
    return false;
  }

  memcpy(out_address_cells, address_cells_prop->value,
         address_cells_prop->value_len);
  memcpy(out_size_cells, size_cells_prop->value, size_cells_prop->value_len);

  *out_address_cells = big_to_native(*out_address_cells);
  *out_size_cells = big_to_native(*out_size_cells);
  return true;
}

bool devicetree_prop_reg(struct devicetree_prop *prop, usize i, paddr *out_addr,
                         usize *out_size) {
  u32 address_cells, size_cells;
  if (!prop || !prop->node || !prop->node->parent)
    return false;
  if (!devicetree_address_size_cells(prop->node->parent, &address_cells,
                                     &size_cells))
    return false;
  assert(address_cells == 1 || address_cells == 2);
  assert(size_cells == 1 || size_cells == 2);

  usize bytes_per_entry = (address_cells + size_cells) * 4;
  if (prop->value_len % bytes_per_entry != 0)
    return false;

  u8 *entry = prop->value + (address_cells + size_cells) * 4 * i;
  if (entry >= prop->value + prop->value_len)
    return false;

  switch (address_cells) {
  case 1: {
    u32 address32;
    memcpy(&address32, entry, 4);
    *out_addr = paddr_of_bits(big_to_native(address32));
    entry += 4;
  } break;
  case 2: {
    u64 address64;
    memcpy(&address64, entry, 8);
    *out_addr = paddr_of_bits(big_to_native(address64));
    entry += 8;
  } break;
  default:
    return false;
  }
  switch (size_cells) {
  case 1: {
    u32 size32;
    memcpy(&size32, entry, 4);
    *out_size = big_to_native(size32);
    entry += 4;
  } break;
  case 2: {
    u64 size64;
    memcpy(&size64, entry, 8);
    *out_size = big_to_native(size64);
    entry += 8;
  } break;
  default:
    return false;
  }

  return true;
}

void devicetree_print(struct devicetree_node *node) {
  if (!node) {
    print("nullptr");
    return;
  }

  print("/ {{");
  usize depth = 2;
  while (depth && node) {
    // Print each property.
    for (struct list_head *prop_list = node->props.next;
         prop_list != &node->props; prop_list = prop_list->next) {
      struct devicetree_prop *prop =
          container_of(prop_list, struct devicetree_prop, node_props_head);
      print("{indent}{cstr} = [{bytes}];", depth, prop->name, prop->value,
            prop->value_len);
    }

    if (node->children.next != &node->children) {
      // Traverse into the first child, if there is one.
      node = container_of(node->children.next, struct devicetree_node,
                          parent_children_head);
      print("{indent}{cstr} {{", depth, node->name);
      depth += 2;
    } else {
      // Go to the parent until we have a sibling or hit the root.
      while (depth > 2 && node->parent &&
             node->parent_children_head.next == &node->parent->children) {
        node = node->parent;
        depth -= 2;
        print("{indent}}}", depth);
      }

      if (depth == 2 || !node->parent) {
        // If we're at the root, finish.
        node = node->parent;
        depth -= 2;
        print("{indent}}}", depth);
      } else {
        // Otherwise, traverse into that next sibling.
        node = container_of(node->parent_children_head.next,
                            struct devicetree_node, parent_children_head);
        print("{indent}}}", depth - 2);
        print("{indent}{cstr} {{", depth - 2, node->name);
      }
    }
  }
}

void devicetree_add_entropy(struct devicetree_node *root) {
  struct devicetree_node *chosen = devicetree_child(root, "chosen");
  if (!chosen)
    return;
  struct devicetree_prop *rng_seed = devicetree_prop(chosen, "rng-seed");
  if (!rng_seed)
    return;

  entropy_pool_mix(rng_seed->value, rng_seed->value_len);
  entropy_pool_credit(8 * rng_seed->value_len);
  devicetree_prop_free(rng_seed);
}
