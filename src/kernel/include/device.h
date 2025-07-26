/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__DEVICE_H
#define UKO_OS_KERNEL__DEVICE_H 1

#include <list.h>
#include <print.h>

/**
 * The kind of device. These should be statically allocated.
 */
struct device_class {
  /**
   * The name of the device class. This should be a short name suitable as a
   * directory name.
   */
  const char *name;

  /**
   * A short description of the device class in prose.
   */
  const char *description;

  /**
   * The `list_head` for this class's entry in the `device_classes` list.
   */
  struct list_head device_classes;

  /**
   * The list of `struct device`s that have this class.
   */
  struct list_head members;
};

/**
 * Defines a device class and creates an initializer to register it.
 */
#define DEFINE_DEVICE_CLASS(IDENTIFIER, NAME, DESCRIPTION)                     \
  struct device_class IDENTIFIER;                                              \
  DEFINE_INIT(-20) { register_device_class(&IDENTIFIER); }                     \
  struct device_class IDENTIFIER = {                                           \
      .name = (NAME),                                                          \
      .description = (DESCRIPTION),                                            \
      .device_classes = LIST_INIT((IDENTIFIER).device_classes),                \
      .members = LIST_INIT((IDENTIFIER).members),                              \
  }

/**
 * The list of registered `struct device_class`es.
 */
extern struct list_head device_classes;

/**
 * Registers a device class.
 *
 * - `name` and `description` must be filled out.
 * - `device_classes` and `members` must be self-linked.
 */
[[gnu::nonnull(1)]]
void register_device_class(struct device_class *device_class);

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

  /**
   * The kind of device this is.
   */
  struct device_class *device_class;

  /**
   * The `list_head` for this device's entry in `device_class->members`.
   */
  struct list_head device_class_members;
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
    .children = LIST_INIT((DEVICE).children), .device_class = nullptr,         \
    .device_class_members = LIST_INIT((DEVICE).device_class_members),          \
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
[[gnu::nonnull(1, 2, 3)]]
void register_device(struct device *parent, struct device_class *device_class,
                     struct device *child);

/**
 * Prints the tree of devices.
 */
void print_devices(void);

#endif // UKO_OS_KERNEL__DEVICE_H
