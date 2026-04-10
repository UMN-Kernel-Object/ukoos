/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__PCI_H
#define UKO_OS_KERNEL__PCI_H 1

#include <device.h>
#include <types.h>

struct pci_regs {
  u16 vid;
  u16 did;
  u8 cmd;
  u8 pad[15];
  u32 memar;
};

struct pci {
  struct list_head list;
  const struct pci_ops *ops;
  struct device *device;
};

struct pci_ops {
  struct vma *(*mmio_alloc)(struct pci *this, usize len, bool want_low_addr);
};

extern struct list_head pcis;

void (*pci_get_handler(u16 vid, u16 did))(struct pci *this,
                                          struct pci_regs *regs);
void pci_register(u16 vid, u16 did,
                  void (*callback)(struct pci *this, struct pci_regs *regs));

enum pci_bus_addr_space_code {
  SPACE_CODE_CONFIG = 0,
  SPACE_CODE_IO = 1,
  SPACE_CODE_MMIO_32 = 2,
  SPACE_CODE_MMIO_64 = 3
};

union pci_bus_addr {
  struct {
    u32 pad : 24;
    enum pci_bus_addr_space_code space_code : 2;
    u32 pad2 : 6;
  };
  u32 bits;
};

#endif
