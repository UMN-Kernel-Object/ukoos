/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <net/icmpv6.h>
#include <net/ipv6.h>
#include <net/udp.h>

struct icmpv6_header icmpv6_create_header(struct ip_address src,
                                          struct ip_address dst, u8 *data,
                                          usize len, u8 type, u8 code) {
  struct ipv6_header pseudo_header = ipv6_create_header(src, dst, ICMP);
  pseudo_header.hop_limit = 255;
  u16 packet[32];
  memcpy(packet, &pseudo_header, sizeof(struct ipv6_header));
  memcpy(packet, &data, len);
  u16 cksum = calc_chksum(packet, sizeof(struct ipv6_header) + len);

  struct icmpv6_header header = {.type = type, .code = code, .cksum = cksum};

  return header;
}
