/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <devices/pci.h>

struct list_head pcis = LIST_INIT(pcis);

struct registered_device {
  void (*callback)(struct pci_regs *regs);
  u16 vid;
  u16 did;
};

struct registered_device *handlers = nullptr;
usize handlers_len = 0, handlers_cap = 0;

void pci_register(u16 vid, u16 did, void (*callback)(struct pci_regs *regs)) {
  if (!handlers) {
    assert(!handlers_len);
    handlers_cap = 16;
    handlers = alloc(sizeof(struct registered_device) * handlers_cap);
    assert(handlers, "Failed to allocate handlers vector");
  }

  if (handlers_cap == handlers_len) {
    handlers_cap *= 2;
    struct registered_device *new_handlers =
        realloc(handlers, sizeof(struct registered_device) * handlers_cap);
    assert(new_handlers, "Failed to reallocate pci handlers vector");
    handlers = new_handlers;
  }

  handlers[handlers_len++] = (struct registered_device) {
      .vid = vid,
      .did = did,
      .callback = callback
  };
}

void (*pci_get_handler(u16 vid, u16 did))(struct pci_regs *regs) {
  for (usize i = 0; i<handlers_len; ++i) {
    if (handlers[i].vid == vid && handlers[i].did == did)
      return handlers[i].callback;
  }
  return nullptr;
}
