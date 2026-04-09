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

int icmpv6_solicit_routers() {
  // address presumably not yet assigned
  struct ip_address src = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

  // all routers multicast
  struct ip_address dst = {{0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2}};

  struct ndp_option opt = {
      .type = NDP_SOURCE_ADDRESS,
      .length = sizeof(struct ndp_option),
  };

  struct icmpv6_header icmpv6_header = icmpv6_create_header(
      src, dst, (u8 *)&opt, sizeof(struct ndp_option), NDP_ROUTER_SOLICIT, 0);

  struct ipv6_header ipv6_header =
      ipv6_create_header(src, dst, NDP_ROUTER_SOLICIT);

  constexpr usize size =
      sizeof(struct icmpv6_header) + sizeof(struct ndp_option);
  u8 buf[size];

  memcpy(buf, &icmpv6_header, sizeof(struct icmpv6_header));
  memcpy(buf, &opt, sizeof(struct ndp_option));

  ipv6_send_packet(ipv6_header, buf, size);

  return 0;
}
