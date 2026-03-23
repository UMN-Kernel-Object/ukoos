/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <mm/alloc.h>
#include <net/icmpv6.h>
#include <net/ipv6.h>

static const struct ip_address LINK_LOCAL = {
    .addr = {0xFE, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x12, 0x34, 0x56, 0x78,
             0x9a, 0xbc, 0xde, 0xf0}};
static const struct ip_address ALL_ROUTERS = {
    .addr = {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
             0x0, 0x0, 0x2}};
static const struct ip_address UNSPECIFIED = {
    .addr = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
             0x0, 0x0, 0x0}};
static const struct ip_address SOLICITED_NODE = {
    .addr = {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0xff,
             0x9a, 0xbc, 0xde}};

#define CEIL(x, n) ((x) / (n) + ((x) % (n)) ? 1 : 0) * (n)

int ndp_discover_routers() {
  struct icmpv6_header icmpv6_header = icmpv6_create_header(
      LINK_LOCAL, ALL_ROUTERS, nullptr, 0, NDP_ROUTER_SOLICIT, 0);
  struct ipv6_header ipv6_header =
      ipv6_create_header(LINK_LOCAL, ALL_ROUTERS, ICMP);
  ipv6_header.hop_limit = 255;
  ipv6_send_packet(ipv6_header, (u8 *)&icmpv6_header, sizeof(icmpv6_header));

  return 0;
};

struct mac_address
ndp_resolve_neighbor_address(struct ip_address src_address,
                             struct ip_address target_address,
                             struct mac_address src_mac_address) {
  // NDP options must be 8 byte aligned
  usize opt_len =
      CEIL(sizeof(struct option_info) + sizeof(struct mac_address), 8);
  usize len = sizeof(struct ndp_neighbor_solicit) + opt_len;
  struct ndp_option option_info = {
      .type = NDP_SRC_ADDR,
      .len = opt_len,
  };
  u8 *message = alloc(len);
  memset(message, 0, opt_len);
  memcpy(message + sizeof(struct ndp_neighbor_solicit), &option_info,
         sizeof(struct ndp_option));
  memcpy(message + sizeof(struct ndp_neighbor_solicit) +
             sizeof(struct ndp_option),
         &src_mac_address, sizeof(struct mac_address));
  memcpy(message + sizeof(struct icmpv6_header), &target_address,
         sizeof(struct ipv6_address)) struct icmpv6_header icmpv6_header =
      icmpv6_create_header(
          src_address, target_address, header + sizeof(struct icmpv6_header),
          len - sizeof(struct icmpv6_header), NDP_NEIGHBOR_SOLICIT, 0);
  struct ipv6_header ipv6_header =
      ipv6_create_header(src_address, target_address, ICMP);
  ipv6_send_packet(ipv6_header, (u8 *)&message, sizeof(message));

  return 0;
}

int ndp_duplicate_address_detection() {
  struct icmpv6_header icmpv6_header = icmpv6_create_header(
      UNSPECIFIED, SOLICITED_NODE, nullptr, 0, NDP_NEIGHBOR_SOLICIT, 0);
  struct ipv6_header ipv6_header =
      ipv6_create_header(UNSPECIFIED, SOLICITED_NODE, ICMP);
  ipv6_header.hop_limit = 255;
  ipv6_send_packet(ipv6_header, (u8 *)&icmpv6_header, sizeof(icmpv6_header));

  return 0;
};
