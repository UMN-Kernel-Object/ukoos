/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__DEVICES_NETDEV_H
#define UKO_OS_KERNEL__DEVICES_NETDEV_H 1

#include <device.h>

struct netdev {
  struct list_head list;
  const struct netdev_ops *ops;
  struct device *device;
};

struct netdev_ops {
  bool (*send_packet)(struct netdev *this, u8 packet[], usize length);
  struct mac (*get_mac)(struct netdev *this);
};

extern struct list_head netdevs;

#endif // UKO_OS_KERNEL__DEVICES_NETDEV_H
