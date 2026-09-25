/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <types.h>
#include <net/ipv6.h>

u16 inet_checksum(u8 *data, usize count);
u16 inet_checksum_with_ip_pseudo_header(struct ip_address src,
                                        struct ip_address dst, u8 ip_proto_num,
                                        u8 *data, usize data_len);
