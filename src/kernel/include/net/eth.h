/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <devices/netdev.h>

#ifndef UKO_OS_KERNEL__NET_ETH__H
#define UKO_OS_KERNEL__NET_ETH__H 1

struct mac {
	u8 addr[6];
};

bool eth_send_packet(struct netdev *device, const struct mac dst, u8 *buffer, usize len);

#endif
