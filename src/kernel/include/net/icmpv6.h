#pragma once

#include <net/ipv6.h>

struct icmp_message_header {
  u8 type;
  u8 code;
  u16 checksum;
};

struct icmp_echo_request {
  u16 identifier;
  u16 sequence_number;
};

enum icmp_message_type {
  ICMP_DESTINATION_UNREACHABLE = 1,
  ICMP_PACKET_TOO_BIG = 2,
  ICMP_TIME_EXCEEDED = 3,
  ICMP_PARAM_PROBLEM = 4,
  ICMP_ECHO_REQUEST = 128,
  ICMP_ECHO_REPLY = 129,
};

u64 icmp_send_message(struct ip_address src, struct ip_address dst, u8 type,
                      u8 code, u8 *data, usize data_len);
u64 icmp_send_echo_request(struct ip_address src, struct ip_address dst,
                           u16 ident, u16 sequence_number, u8 *data,
                           usize data_len);
