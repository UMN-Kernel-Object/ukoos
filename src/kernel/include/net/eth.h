/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__NET_ETH__H
#define UKO_OS_KERNEL__NET_ETH__H 1

bool eth_send_packet(struct netdev *device, u8 *buffer, u32 len);

#endif
