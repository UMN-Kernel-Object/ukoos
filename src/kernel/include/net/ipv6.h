/* SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_NET_IPV6_H
#define UKO_OS_NET_IPV6_H 1

#include <types.h>

// implementation of https://datatracker.ietf.org/doc/html/rfc8200

enum ip_next_protocol_value : u8 {
  NET_PROTOCOL_HOPOPT = 0,
  NET_PROTOCOL_ICMP = 58,
  NET_PROTOCOL_TCP = 6,
  NET_PROTOCOL_UDP = 17
};

struct ip_address {
  u8 bytes[16];
};

struct ip_header {
  u8 vtf[4];
  u16 payload_len;
  u8 next_header;
  u8 hop_limit;
  struct ip_address src;
  struct ip_address dst;
};

static_assert(sizeof(struct ip_header) == 40);

struct hop_by_hop_opt_header {
  u8 next_header;
  u8 hdr_ext_len;
  u8 *options; // https://datatracker.ietf.org/doc/html/rfc8200#section-4.3
};

// unusued, we aren't routing
// struct routing_header {
//  u8 next_header;
//  u8 hdr_ext_len;
//  u8 routing_type;
//  u8 segments_left;
//};

struct fragment_header {
  u8 next_header;
  u8 reserved;
  u16 fragment_offset : 13;
  u8 res : 2;
  u8 m : 1;
  u32 identifcation;
};

// unused, we will be ignoring having nodes.
// struct dst_options_header {
//  u8 next_header;
//  u8 hdr_ext_len;
//  u8 *options; // https://datatracker.ietf.org/doc/html/rfc8200#section-4.6
//}
//

u64 ip_send_packet(struct ip_header header, u8 *data, usize len);

struct ip_header ip_create_header(struct ip_address src_address,
                                  struct ip_address dst_address,
                                  enum ip_next_protocol_value protocol);

#endif // UKO_OS_NET_IPV6_H
