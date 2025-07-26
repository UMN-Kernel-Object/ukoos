/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <device.h>
#include <init.h>

struct list_head device_classes = LIST_INIT(device_classes);

void register_device_class(struct device_class *device_class) {
  assert(device_class->name);
  assert(device_class->description);
  assert(list_is_empty(&device_class->device_classes));
  assert(list_is_empty(&device_class->members));

  list_push(&device_classes, &device_class->device_classes);
}

void register_device(struct device *parent, struct device_class *device_class,
                     struct device *child) {
  assert(parent->name);
  assert(parent->device_class);
  assert(!list_is_empty(&parent->device_class_members));

  assert(child->name);
  assert(!child->parent);
  assert(!child->device_class);
  assert(list_is_empty(&child->parent_children));
  assert(list_is_empty(&child->children));
  assert(list_is_empty(&child->device_class_members));

  child->parent = parent;
  child->device_class = device_class;
  list_push(&parent->children, &child->parent_children);
  list_push(&device_class->members, &child->device_class_members);
}

void print_devices(void) {
  struct device *device = &root_device;
  usize depth = 1;

  print("devices {{");

  assert(list_is_empty(&device->parent_children));
  for (;;) {
    assert(device->name);
    assert(device->device_class);
    assert(device->device_class->name);
    print("{indent}{cstr} class={cstr}{cstr}", 2 * depth, device->name,
          device->device_class->name,
          list_is_empty(&device->children) ? "" : " {");

    if (!list_is_empty(&device->children)) {
      // Try to go to our first child.
      depth++;
      device =
          container_of(device->children.next, struct device, parent_children);
    } else {
      // Go up the tree until we find a node with a next sibling, or we've
      // returned to the root.
      while (device->parent &&
             device->parent_children.next == &device->parent->children) {
        device = device->parent;
        assert(depth > 1);
        depth--;
        print("{indent}}}", 2 * depth);
      }

      // If we're at the root, stop traversing.
      if (!device->parent) {
        assert(depth == 1);
        break;
      }

      // Otherwise, go to our sibling.
      device = container_of(device->parent_children.next, struct device,
                            parent_children);
    }
  }

  print("}}");
}

/**
 * The device class of the root device. No other device should have this device
 * class.
 */
DEFINE_DEVICE_CLASS(root_device_class, "ukoos-root-device",
                    "The device class of the root device.");

struct device root_device;

DEFINE_INIT(-10) {
  root_device = DEVICE_INIT(root_device, "root-device");
  // This can't use register_device() because it doesn't have a parent.
  root_device.device_class = &root_device_class;
  list_push(&root_device_class.members, &root_device.device_class_members);
}
