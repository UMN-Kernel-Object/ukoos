/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__DEVICE_H
#define UKO_OS_KERNEL__DEVICE_H 1

#include <init.h>
#include <list.h>
#include <print.h>

/**
 * A node in the tree of devices attached to the system.
 */
struct device {
  /**
   * The name of the device. This must be unique amongst its siblings.
   *
   * This should be `alloc`-allocated.
   */
  char *name;

  /**
   * The parent of the node. `nullptr` for the root node.
   */
  struct device *parent;

  /**
   * The `list_head` for this device's entry in `parent->children`.
   */
  struct list_head parent_children;

  /**
   * The children of the node.
   */
  struct list_head children;
};

/**
 * Returns an initializer for the `struct device` `DEVICE`.
 *
 * ### Example
 *
 * ```c
 * struct my_device {
 *   // ...
 *   struct device device;
 *   // ...
 * };
 *
 * struct my_device *my_device = alloc(sizeof(*my_device));
 * my_device->device = DEVICE_INIT(my_device->device, "example");
 * ```
 */
#define DEVICE_INIT(DEVICE, NAME_FORMAT, ...)                                  \
  (struct device) {                                                            \
    .name = format(NAME_FORMAT __VA_OPT__(, ) __VA_ARGS__), .parent = nullptr, \
    .parent_children = LIST_INIT((DEVICE).parent_children),                    \
    .children = LIST_INIT((DEVICE).children),                                  \
  }

/**
 * The root of the tree of devices.
 */
extern struct device root_device;

/**
 * Registers a device in the tree of devices and with its device class.
 *
 * - `name` must be filled out.
 * - `parent` and `device_class` must be `nullptr`.
 * - `parent_children`, `children`, and `device_class_members` must be
 *   self-linked.
 */
[[gnu::nonnull(1, 2)]]
void device_add(struct device *parent, struct device *child);

/**
 * Prints the tree of devices.
 */
void print_devices(void);

#endif // UKO_OS_KERNEL__DEVICE_H
