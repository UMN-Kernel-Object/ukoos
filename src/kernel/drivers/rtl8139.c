/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <mm/paging.h>
#include <mm/virtual_alloc.h>
#include <physical.h>
#include <print.h>

#include <device.h>
#include <devices/pci.h>
#include <devices/netdev.h>
#include <net/eth.h>

struct rtl8139_regs {
  u8 mac[6];
  u8 pad0[10];
  u32 tsd[4];
  u32 tsad[4];
  u8 pad1[7];
  u8 com;
  u8 pad2[26];
  u8 config1;
};

struct rtl8139 {
  struct device device;
  struct netdev netdev;
  struct rtl8139_regs *regs;
  usize current_buffer;
};

bool send_packet(struct netdev *this, u8 packet[], usize length);
struct mac get_mac(struct netdev *this);

struct netdev_ops rtl8139_ops = {
  .send_packet = send_packet,
  .get_mac = get_mac
};

constexpr usize TX_BUFF_SIZE = 0x1700;
constexpr u16 rtl8139_vid = 0x10ec;
constexpr u16 rtl8139_did = 0x8139;

enum rtl8139_registers {
  REG_COM = 0x37,
  REG_TCR = 0x40,
  REG_CONFIG1 = 0x52
};

enum rtl8139_flags {
  TSD_TOK = (1 << 15),
  TSD_OWN = (1 << 13),

  COM_RST = (1 << 4),
  COM_RE = (1 << 3),
  COM_TE = (1 << 2)
};

u8 rtl8139_tbuff[4][TX_BUFF_SIZE] __attribute__((aligned(256)));

static_assert( offsetof(struct rtl8139_regs, tsad) == 0x20);
static_assert( offsetof(struct rtl8139_regs, tsd) == 0x10);
static_assert( offsetof(struct rtl8139_regs, com) == 0x37);
static_assert( offsetof(struct rtl8139_regs, config1) == 0x52);

bool mm_paging_walk(uaddr va, paddr *pte, bool alloc);

u32 walkaddr(uaddr addr) {
  paddr pte_addr;
  u64 pte;
  u32 off;

  off = addr & 0xfff;
  addr &= (~0xfffUL);

  assert(mm_paging_walk(addr, &pte_addr, false));

  copy_from_physical(&pte, pte_addr, sizeof pte);

  pte = (pte >> 10) << 12;
  assert(pte < U32_MAX);
  return (u32) pte | off;
}

struct mac get_mac(struct netdev *this) {
  struct rtl8139 *rtl_this = (struct rtl8139 *)(this->device);
  volatile struct rtl8139_regs *rtl_regs = rtl_this->regs;
  struct mac mac;

  for (usize i = 0; i<6; ++i) {
    mac.addr[i] = rtl_regs->mac[i];
  }
  return mac;
}

bool send_packet(struct netdev *this, u8 packet[], usize length) {
  struct rtl8139 *rtl_this = container_of(this, struct rtl8139, netdev);
  volatile struct rtl8139_regs *rtl_regs = rtl_this->regs;
  u32 phys_buffer = walkaddr((uaddr) (&rtl8139_tbuff[0]));
  u64 i;

  for (i=0; i<length; ++i) {
    rtl8139_tbuff[rtl_this->current_buffer][i] = packet[i];
  }

  if ((rtl_regs->tsd[rtl_this->current_buffer] & TSD_OWN) == 0) {
    return false;
  }

  rtl_regs->tsad[rtl_this->current_buffer] = phys_buffer;
  rtl_regs->tsd[rtl_this->current_buffer] = (u32) length;

  ++rtl_this->current_buffer;
  return true;
}


void rtl8139_test(struct rtl8139 *rtl_device) {
  volatile struct rtl8139_regs *rtl_regs = rtl_device->regs;
  struct mac mac;
  memcpy(mac.addr, (const u8*) "\xff\xff\xff\xff\xff\xff", 6);

  eth_send_packet(&rtl_device->netdev, mac,
    (u8*) "yellow submarine", 16);

  assert(rtl_regs->tsad[0] != 0);
  assert((rtl_regs->tsd[0] & TSD_TOK) != 0);
}


static struct device *add_device(paddr reg_addr, usize reg_size) {
  struct rtl8139 *device = nullptr;

  if (reg_size < sizeof(struct rtl8139_regs))
    goto fail;

  device = alloc(sizeof(struct rtl8139));
  if (!device)
    goto fail;

  *device = (struct rtl8139){
      .device = DEVICE_INIT(device->device, "rtl8139", reg_addr),
      .netdev =
          {
              .list = LIST_INIT(device->netdev.list),
              .ops = &rtl8139_ops,
              .device = &device->device,
          },
      .regs = iomem_map(reg_addr, reg_size),
  };
  if (!device->device.name || !device->regs)
    goto fail;

  list_push(&netdevs, &device->netdev.list);

  return &device->device;

fail:
  if (device) {
    free(device->device.name);
    iomem_unmap(device->regs, reg_size);
  }
  free(device);
  return nullptr;

}

void rtl8139_init(struct pci* pci, struct pci_regs *pci_device) {
  print("initializing rtl8139");

  volatile struct rtl8139_regs *rtl_regs;
  u32 reg_addr = 0;
  struct vma *mmio_addr = pci->ops->mmio_alloc(pci, 256, 1);
  uaddr low, high;

  vma_bounds(mmio_addr, &low, &high);

  /* Should always be true, see mmio_alloc */
  assert(low < U32_MAX);
  reg_addr = (u32) low;

  struct device *device = add_device(paddr_of_bits(reg_addr), sizeof(struct rtl8139_regs));
  struct rtl8139 *rtl_device = (struct rtl8139 *) device;
  rtl_regs = rtl_device->regs;

  /* TODO: Maybe move PCI init to pci.c? */
  /* Setup BAR for MMIO - we just pick a nice number for now */
  pci_device->memar = reg_addr;
  pci_device->cmd |= 0x6;

  rtl_regs->config1 = 0;

  rtl_regs->com = COM_RST;
  while (rtl_regs->com & COM_RST) ;
  rtl_regs->com = COM_RE | COM_TE;

  rtl_regs->tsad[0] = 0x12;
  assert(rtl_regs->tsad[0] == 0x12);

  rtl8139_test(rtl_device);
}

DEFINE_INIT(INIT_REGISTER_PCI_DRIVERS) {
  pci_register(rtl8139_vid, rtl8139_did, rtl8139_init);
}
