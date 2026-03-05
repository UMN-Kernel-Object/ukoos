/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <device.h>
#include <init.h>

void device_add(struct device *parent, struct device *child) {
  assert(parent->name);

  assert(child->name);
  assert(!child->parent);
  assert(list_is_empty(&child->parent_children));
  assert(list_is_empty(&child->children));

  child->parent = parent;
  list_push(&parent->children, &child->parent_children);
}

void print_devices(void) {
  struct device *device = &root_device;
  usize depth = 1;

  print("devices {{");

  assert(list_is_empty(&device->parent_children));
  for (;;) {
    assert(device->name);
    print("{indent}{cstr} {cstr}", 2 * depth, device->name,
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

struct device root_device = {0};

// Despite not being a driver, we initialize the root device here. We know we
// won't be creating devices yet, so it's a convenient point to do so.
DEFINE_INIT(INIT_REGISTER_DRIVERS) {
  root_device = DEVICE_INIT(root_device, "root-device");
}
