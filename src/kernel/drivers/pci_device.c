#include <print.h>
#include <mm/paging.h>
#include <mm/virtual_alloc.h>
#include <physical.h>
#include <devicetree.h>

//#include <drivers/rtl8139.h>
#include <devices/pci.h>

//constexpr u64 pci_config = 0x30000000L;



struct pci_device {
  struct device device;
  struct pci pci;
  void *reg_base;
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

  for (u64 i=0; i<32; ++i) {
    //mm_paging_map(lo + i * 0x1000, paddr_of_bits(pci_config + i * 0x1000), PGPERM_KRW);
    //mm_paging_map(lo + i * 0x1000, paddr_offset(pci_config, i * 0x1000), PGPERM_KRW);
  }
  for (u64 dev=0; dev<32; ++dev) {
    u64 bus = 0;
    u64 func = 0;
    u64 offset = 0;
    offset = (bus << 16) | (dev << 11) | (func << 8) | (offset);
    struct pci_regs *device = (struct pci_regs *)(reg_base + offset);

    u16 did = device->did;
    u16 vid = device->vid;

    void (*callback)(struct pci_regs *) = pci_get_handler(vid, did);
    
    if (callback)  {
      callback(device);
    } else {
      print("unknown pci device: {u16:x} vender: {u16:x}", did, vid);
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

/*DEFINE_INIT(INIT_PCI_DEVICES) {
  // TODO: iterate through registered devices, enumerate PCI bus
  struct pci *pci = (struct pci*) pcis.next;
  struct device *dev = pci->device;
  struct pci_device *pcidev = (void*) dev;
  print("Ready to init pci {u64:x}", pcidev);
}*/
