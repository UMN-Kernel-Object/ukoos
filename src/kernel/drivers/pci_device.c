/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <print.h>
#include <mm/paging.h>
#include <mm/virtual_alloc.h>
#include <physical.h>
#include <devicetree.h>
#include <volatile.h>
#include <container.h>
#include <endian.h>

#include <devices/pci.h>

struct pci_device {
  struct device device;
  struct pci pci;
  void *reg_base;
  struct vma_allocator *mmio_low_allocator;
  struct vma_allocator *mmio_high_allocator;
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

void register_device(struct pci *this, u16 vid, u16 did, void (*callback)(struct pci* this, void *regs));
struct vma* mmio_alloc(struct pci *this, usize len, bool want_low_addr);

static struct pci_ops pci_device_ops = {
  .mmio_alloc = mmio_alloc
};
static_assert( offsetof(struct pci_regs, memar) == 0x14);

struct vma* mmio_alloc(struct pci *this, usize num_pages, bool want_low_addr) {
  struct pci_device *dev = container_of(this, struct pci_device, pci);
  struct vma_allocator *allocator;

  if (want_low_addr) {
    allocator = dev->mmio_low_allocator;
  } else {
    allocator = dev->mmio_high_allocator;
  }

  return vma_alloc(allocator, num_pages);
}

void pci_enumerate(struct pci *this, void *reg_base) {

  for (u32 bus=0; bus<256; ++bus) {
    for (u32 dev=0; dev<32; ++dev) {
      union pci_device_addr addr;
      addr.offset = 0;
      addr.func = 0;
      addr.dev = dev & 31;
      addr.bus = bus & 255;

      struct pci_regs *device = (struct pci_regs *)(reg_base + addr.bits);

      u16 did = READ_ONCE(&device->did);
      u16 vid = READ_ONCE(&device->vid);

      if (did == 0xffff && vid == 0xffff)
        continue;

      void (*callback)(struct pci*, struct pci_regs *) = pci_get_handler(vid, did);

      if (callback)  {
        callback(this, device);
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
  u32 address_cells, size_cells;
  u32 parent_address_cells, parent_size_cells;

  uaddr mmio_32bit_low, mmio_32bit_high;
  uaddr mmio_64bit_low, mmio_64bit_high;

  if (!devicetree_reg(node, 0, &reg_addr, &reg_size))
    goto fail;

  if (reg_size < sizeof(struct pci_regs))
    goto fail;

  device = alloc(sizeof(struct pci_device));
  if (!device)
    goto fail;

  if (!devicetree_address_size_cells(node, &address_cells, &size_cells))
    goto fail;

  if (!devicetree_address_size_cells(node->parent, &parent_address_cells,
                                     &parent_size_cells))
    goto fail;


  devicetree_print(node->parent);

  usize triplet_size = (address_cells + parent_address_cells + size_cells) * 4;
  struct devicetree_prop *base = devicetree_prop(node, "ranges");

  /* TODO: when we allow adding ranges of memory to a VMA allocator, make this
   * add each range as we iterate. Currently, this PCI device only has 1 of
   * each, so it is not an issue.
   */
  mmio_32bit_low = 0;
  mmio_32bit_high = 0;
  mmio_64bit_low = 0;
  mmio_64bit_high = 0;

  for (usize i = 0; i < base->value_len; i += triplet_size) {
    union pci_bus_addr bus_addr;
    u64 addr, size;

    memcpy(&bus_addr.bits, &base->value[i], 4);
    memcpy(&addr, &base->value[i+address_cells*4], parent_address_cells * 4);
    memcpy(&size, &base->value[i+(address_cells + parent_address_cells)*4], size_cells * 4);

    addr = big_to_native(addr);
    size = big_to_native(size);
    bus_addr.bits = big_to_native(bus_addr.bits);

    switch (bus_addr.space_code) {
    case SPACE_CODE_MMIO_32:
      mmio_32bit_low = addr;
      mmio_32bit_high = addr + size;
      break;
    case SPACE_CODE_MMIO_64:
      mmio_64bit_low = addr;
      mmio_64bit_high = addr + size;
      break;
    default:
      /* SPACE_CODE_IO and CONFIG ignored*/
    }
  }

  if (base)
    print("{u64}", base->value_len);

  *device = (struct pci_device){
      .device = DEVICE_INIT(device->device, "pci@{paddr}", reg_addr),
      .pci =
          {
              .list = LIST_INIT(device->pci.list),
              .ops = &pci_device_ops,
              .device = &device->device,
          },
      .reg_base = iomem_map(reg_addr, reg_size),
      .mmio_low_allocator = vma_allocator_new(mmio_32bit_low, mmio_32bit_high),
      .mmio_high_allocator = vma_allocator_new(mmio_64bit_low, mmio_64bit_high)
  };

  if (!device->device.name || !device->reg_base)
    goto fail;

  if (!device->mmio_low_allocator || !device->mmio_high_allocator)
    goto fail;

  list_push(&pcis, &device->pci.list);

  pci_enumerate(&device->pci, device->reg_base);

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
