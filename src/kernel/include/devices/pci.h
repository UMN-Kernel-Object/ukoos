#ifndef UKO_OS_KERNEL__PCI_H
#define UKO_OS_KERNEL__PCI_H 1

#include <types.h>
#include <device.h>


//void pci_enumerate();

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

struct pci_ops { };

extern struct list_head pcis;

void (*pci_get_handler(u16 vid, u16 did))(struct pci_regs *regs);
void pci_register(u16 vid, u16 did, void (*callback)(struct pci_regs *regs));

#endif
