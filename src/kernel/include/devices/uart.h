/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__DEVICES_UART_H
#define UKO_OS_KERNEL__DEVICES_UART_H 1

#include <device.h>

/**
 * A UART device.
 */
struct uart {
  /**
   * The list head used to link the uart into the `uarts` list.
   */
  struct list_head list;

  /**
   * The vtable.
   */
  const struct uart_ops *ops;

  /**
   * A pointer to the `struct device` for this UART.
   */
  struct device *device;
};

/**
 * The vtable for a UART.
 */
struct uart_ops {
  /**
   * Writes a single byte to the UART.
   *
   * TODO: This should be replaced by a more DMA-friendly API.
   */
  void (*write_byte)(struct uart *this, u8 byte);
};

/**
 * The list of all UART devices.
 */
extern struct list_head uarts;

#endif // UKO_OS_KERNEL__DEVICES_UART_H
