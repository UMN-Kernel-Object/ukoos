/* SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <types.h>

// implementation of https://datatracker.ietf.org/doc/html/rfc8200

enum IPv6ProtoNum : u8 { HOPOPT = 0, ICMP = 1, TCP = 6, UDP = 17 };

struct ipv6_header {
  u8 version : 4;
  u8 traffic_class;
  u8 flow_label : 20;
  u16 payload_length;
  u8 next_header;
  u8 hop_limit;
  u128 src_address;
  u128 dst_address;
};

struct hop_by_hop_opt_header {
  u8 next_header;
  u8 hdr_ext_len;
  u8 *options; // https://datatracker.ietf.org/doc/html/rfc8200#section-4.3
};

struct routing_header {
  u8 next_header;
  u8 hdr_ext_len;
  u8 routing_type;
  u8 segments_left;
  // TODO figure out what to do about type-specific data here:
  // https://datatracker.ietf.org/doc/html/rfc8200#section-4.4
};

struct fragment_header {
  u8 next_header;
  u8 reserved;
  u16 fragment_offset : 13;
  u8 res : 2;
  u8 m : 1;
  u32 identifcation;
};

struct dst_options_header {
  u8 next_header;
  u8 hdr_ext_len;
  u8 *options; // https://datatracker.ietf.org/doc/html/rfc8200#section-4.6
}
