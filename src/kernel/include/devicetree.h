/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__DEVICETREE_H
#define UKO_OS_KERNEL__DEVICETREE_H 1

#include <device.h>

/**
 * A property in the Devicetree.
 */
struct devicetree_prop {
  /**
   * This node that has this property. Non-owning, never `nullptr`.
   */
  struct devicetree_node *node;

  /**
   * The `list_head` for this property's entry in `node->props`.
   */
  struct list_head node_props_head;

  /**
   * The name of this property. Null-terminated.
   */
  char *name;

  /**
   * The length of the value field.
   */
  usize value_len;

  /**
   * The value of this property.
   */
  u8 *value;
};

/**
 * A node in the Devicetree.
 */
struct devicetree_node {
  /**
   * This parent of this node. Non-owning, `nullptr` for the root node.
   */
  struct devicetree_node *parent;

  /**
   * The `list_head` for this node's entry in `parent->children`.
   */
  struct list_head parent_children_head;

  /**
   * The nodes that are children of this node.
   */
  struct list_head children;

  /**
   * The props of this node.
   */
  struct list_head props;

  /**
   * The name of this node. Null-terminated.
   */
  char *name;
};

/**
 * Parses a Devicetree, given its start address in _physical_ memory.
 */
struct devicetree_node *devicetree_parse_from_physical(paddr devicetree_start,
                                                       paddr kernel_start,
                                                       paddr kernel_end);

/**
 * Frees the Devicetree.
 */
void devicetree_free(struct devicetree_node *devicetree);

/**
 * Finds the child of the Devicetree node with the given name. Returns `nullptr`
 * if there is no such child.
 */
struct devicetree_node *devicetree_child(struct devicetree_node *parent,
                                         const char *name);

/**
 * Finds the property of the Devicetree node with the given name. Returns
 * `nullptr` if there is no such property.
 */
struct devicetree_prop *devicetree_prop(struct devicetree_node *parent,
                                        const char *name);

/**
 * Retrieves the `#address-cells` and `#size-cells` properties of a node.
 * Returns whether this succeeded.
 */
bool devicetree_address_size_cells(struct devicetree_node *node,
                                   u32 *out_address_cells, u32 *out_size_cells);

/**
 * Retrieves the `i`th address and size pair from the `reg` property of a node.
 * Returns whether this succeeded.
 */
bool devicetree_reg(struct devicetree_node *node, usize i, paddr *out_addr,
                    usize *out_size);

/**
 * Retrieves the `i`th address and size pair from a property. This is useful
 * for properties like `reg`. Returns whether this succeeded.
 */
bool devicetree_prop_reg(struct devicetree_prop *prop, usize i, paddr *out_addr,
                         usize *out_size);

/**
 * Prints the Devicetree.
 */
void devicetree_print(struct devicetree_node *root);

/**
 * Adds the `/chosen/rng-seed` property to the entropy pool, then deletes it
 * from the Devicetree.
 */
void devicetree_add_entropy(struct devicetree_node *root);

/**
 * Walks the Devicetree. On each node with a `compatible` prop, all the handlers
 * registered with `devicetree_register` that could apply are called until one
 * returns non-`nullptr`.
 */
void devicetree_enumerate(struct devicetree_node *root);

/**
 * Adds a handler for devices with the given value for the `compatible` prop,
 * for use by `devicetree_enumerate`.
 */
void devicetree_register(
    const char *compatible,
    struct device *(*handler)(struct devicetree_node *node));

#endif // UKO_OS_KERNEL__DEVICETREE_H
