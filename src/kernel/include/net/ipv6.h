/* SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <types.h>

// implementation of https://datatracker.ietf.org/doc/html/rfc8200

enum IPv6ProtoNum : u8 { HOPOPT = 0, ICMP = 1, TCP = 6, UDP = 17 };

struct ipv6_header {
  u8 vtf[4];
  u16 payload_len;
  u8 next_header;
  u8 hop_limit;
  // TODO: put these in a struct ip_address
  u8 src[16];
  u8 dst[16];
};

static_assert(sizeof(struct ipv6_header) == 48)

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

long ipv6_send_packet(struct ipv6_header header, u8 *data, usize len);
struct ipv6_header ipv6_create_header(u8 src_address[16], u8 dst_address[16],
                                      u8 protocol);
