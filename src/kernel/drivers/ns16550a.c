/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <devices/uart.h>
#include <devicetree.h>
#include <mm/paging.h>
#include <physical.h>

/**
 * The bits of the Interrupt Enable Register.
 */
struct ns16550a_ier {
  /**
   * Enable Receiver Buffer Full Interrupt.
   */
  bool erbfi : 1;

  /**
   * Enable Transmitter Buffer Empty Interrupt.
   */
  bool etbei : 1;

  /**
   * Enable (Receiver) Line Status Interrupt.
   */
  bool elsi : 1;

  /**
   * Enable Modem Status Interrupt.
   */
  bool edssi : 1;

  /**
   * Reserved, should be zero.
   *
   * Wikipedia suggests that the 16750 uses some of these bits.
   */
  u8 rsvd : 4;
};

/**
 * The bits of the Interrupt Identification Register.
 */
struct ns16550a_iir {
  /**
   * A bit that becomes unset when an interrupt is pending.
   */
  bool interrupt_pending : 1;

  /**
   * The index of the bit in `ns16550a_ier` corresponding to the pending
   * interrupt.
   */
  u8 interrupt_id : 3;

  /**
   * Reserved, should be zero.
   *
   * Wikipedia suggests that the 16750 uses some of these bits.
   */
  u8 rsvd : 2;

  /**
   * A flag for whether the FIFOs are currently functioning.
   */
  bool fifo_working : 1;

  /**
   * A flag for whether the FIFOs are enabled.
   */
  bool fifo_enabled : 1;
};

/**
 * The UART's registers.
 */
struct ns16550a_regs {
  union {
    /**
     * The Receiver Buffer Register. Read-only. Only valid when DLAB is unset.
     */
    u8 rbr;

    /**
     * The Transmitter Holding Register. Write-only. Only valid when DLAB is
     * unset.
     */
    u8 thr;

    /**
     * The Divisor Latch Low byte. Only valid when DLAB is set.
     */
    u8 dll;
  };

  /**
   * When DLAB is unset, this is the Interrupt Enable Register. When DLAB is
   * set, this is.
   */
  union {
    /**
     * The Interrupt Enable Register. Only valid when DLAB is unset.
     */
    struct ns16550a_ier ier;

    /**
     * The Divisor Latch High byte. Only valid when DLAB is set.
     */
    u8 dlh;
  };

  union {
    /**
     * The Interrupt Identification Register. Read-only.
     */
    struct ns16550a_iir iir;

    /**
     * The FIFO Control Register. Write-only.
     */
    u8 fcr;
  };

  /**
   * The Line Control Register.
   */
  u8 lcr;

  /**
   * The Modem Control Register.
   */
  u8 mcr;

  /**
   * The Line Status Register.
   */
  u8 lsr;

  /**
   * The Modem Status Register.
   */
  u8 msr;

  /**
   * The Scratch Register.
   */
  u8 sr;
};

static_assert(sizeof(struct ns16550a_regs) == 8);

struct ns16550a {
  struct device device;
  struct uart uart;
  struct ns16550a_regs *regs;
};

static void ns16550a_write_byte(struct uart *uart, u8 byte) { TODO(); }

static const struct uart_ops ns16550a_uart_ops = {
    .write_byte = ns16550a_write_byte,
};

static struct device *ns16550a_enumerate_dt(struct devicetree_node *node) {
  struct ns16550a *device = nullptr;
  paddr reg_addr;
  usize reg_size;

  if (!devicetree_reg(node, 0, &reg_addr, &reg_size))
    goto fail;
  if (reg_size < sizeof(struct ns16550a_regs))
    goto fail;

  device = alloc(sizeof(struct ns16550a));
  if (!device)
    goto fail;

  *device = (struct ns16550a){
      .device = DEVICE_INIT(device->device, "ns16550a@{paddr}", reg_addr),
      .uart =
          {
              .list = LIST_INIT(device->uart.list),
              .ops = &ns16550a_uart_ops,
              .device = &device->device,
          },
      .regs = iomem_map(reg_addr, reg_size),
  };
  if (!device->device.name || !device->regs)
    goto fail;

  list_push(&uarts, &device->uart.list);

  return &device->device;

fail:
  if (device) {
    free(device->device.name);
    iomem_unmap(device->regs, reg_size);
  }
  free(device);
  return nullptr;
}

DEFINE_INIT(INIT_REGISTER_DRIVERS) {
  devicetree_register("ns16550a", ns16550a_enumerate_dt);
}
