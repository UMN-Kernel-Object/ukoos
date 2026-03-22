/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <net/icmpv6.h>
#include <net/ipv6.h>

static const struct ip_address LINK_LOCAL = {
    .addr = {0xFE, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x12, 0x34, 0x56, 0x78,
             0x9a, 0xbc, 0xde, 0xf0}};
static const struct ip_address ALL_ROUTERS = {
    .addr = {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
             0x0, 0x0, 0x2}};

int ndp_discover_routers() {
  struct icmpv6_header icmpv6_header = icmpv6_create_header(
      LINK_LOCAL, ALL_ROUTERS, nullptr, 0, NDP_ROUTER_SOLICIT, 0);
  struct ipv6_header ipv6_header =
      ipv6_create_header(LINK_LOCAL, ALL_ROUTERS, ICMP);
  ipv6_header.hop_limit = 255;
  ipv6_send_packet(ipv6_header, (u8 *)&icmpv6_header, sizeof(icmpv6_header));

  return 0;
};
