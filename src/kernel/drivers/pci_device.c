/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <print.h>
#include <mm/paging.h>
#include <mm/virtual_alloc.h>
#include <physical.h>
#include <devicetree.h>

#include <devices/pci.h>

struct pci_device {
  struct device device;
  struct pci pci;
  void *reg_base;
};
union pci_device_addr {
  struct {
    u32 offset : 8;
    u32 func : 3;
    u32 dev : 5;
    u32 bus : 8;
    u32 rsvd : 7;
    u32 enable : 1;
  };
  u32 bits;
};

void register_device(struct pci *this, u16 vid, u16 did, void (*callback)(void *regs));

static struct pci_ops pci_device_ops = {
};
static_assert( offsetof(struct pci_regs, memar) == 0x14);

void pci_enumerate(uaddr reg_base) {
  uaddr lo, hi;
  struct vma *vma;

  vma = vma_alloc(&kernel_virtual_allocator, 32);
  assert(vma);

  vma_bounds(vma, &lo, &hi);

  for (u32 bus=0; bus<256; ++bus) {
    for (u32 dev=0; dev<32; ++dev) {
      union pci_device_addr addr;
      addr.offset = 0;
      addr.func = 0;
      addr.dev = dev & 31;
      addr.bus = bus & 255;

      struct pci_regs *device = (struct pci_regs *)(reg_base + addr.bits);

      u16 did = device->did;
      u16 vid = device->vid;

      if (did == 0xffff && vid == 0xffff)
        continue;

      void (*callback)(struct pci_regs *) = pci_get_handler(vid, did);

      if (callback)  {
        callback(device);
      } else {
        print("Unbekannt pci Gerät: {u16:x} Anbieter: {u16:x}", did, vid);
      }
    }
  }
}

static struct device *pci_enumerate_dt(struct devicetree_node *node) {
  struct pci_device *device = nullptr;
  paddr reg_addr;
  usize reg_size;

  if (!devicetree_reg(node, 0, &reg_addr, &reg_size))
    goto fail;

  if (reg_size < sizeof(struct pci_regs))
    goto fail;

  device = alloc(sizeof(struct pci_device));
  if (!device)
    goto fail;

  *device = (struct pci_device){
      .device = DEVICE_INIT(device->device, "pci@{paddr}", reg_addr),
      .pci =
          {
              .list = LIST_INIT(device->pci.list),
              .ops = &pci_device_ops,
              .device = &device->device,
          },
      .reg_base = iomem_map(reg_addr, reg_size),
  };
  if (!device->device.name || !device->reg_base)
    goto fail;

  list_push(&pcis, &device->pci.list);

  pci_enumerate((uaddr) device->reg_base);

  return &device->device;

fail:
  if (device) {
    free(device->device.name);
    iomem_unmap(device->reg_base, reg_size);
  }
  free(device);
  return nullptr;
}

DEFINE_INIT(INIT_REGISTER_DRIVERS) {
  devicetree_register("pci-host-ecam-generic", pci_enumerate_dt);
}
