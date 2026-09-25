/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <endian.h>
#include <mm/alloc.h>
#include <net/common.h>
#include <net/icmpv6.h>
#include <panic.h>
#include <print.h>
#include <selftest.h>

u64 icmp_send_message(struct ip_address src, struct ip_address dst, u8 type,
                      u8 code, u8 *data, usize data_len) {
  usize total_len = sizeof(struct icmp_message_header) + data_len;

  assert(total_len < 1UL << 32);

  struct ip_header ip_header = ip_create_header(src, dst, NET_PROTOCOL_ICMP);

  struct icmp_message_header message_header = {
      .type = type,
      .code = code,
      .checksum = 0,
  };

  u8 *packet = alloc(total_len);

  memcpy(packet, &message_header, sizeof(message_header));
  memcpy(packet + sizeof(message_header), data, sizeof(data));

  u16 checksum = inet_checksum_with_ip_pseudo_header(
      src, dst, NET_PROTOCOL_ICMP, packet, total_len);

  message_header.checksum = checksum;
  memcpy(packet, &message_header, sizeof(message_header));

  u64 ret = ip_send_packet(ip_header, packet, total_len);

  free(packet);

  return ret;
}

u64 icmp_send_echo_request(struct ip_address src, struct ip_address dst,
                           u16 identifier, u16 sequence_number, u8 *data,
                           usize data_len) {
  struct icmp_echo_request echo_request = {
      .identifier = native_to_big(identifier),
      .sequence_number = native_to_big(sequence_number),
  };
  usize total_len = sizeof(echo_request) + data_len;

  u8 *packet = alloc(total_len);

  memcpy(packet, &echo_request, sizeof(echo_request));
  memcpy(packet, data, data_len);

  u64 ret =
      icmp_send_message(src, dst, ICMP_ECHO_REQUEST, 0, packet, total_len);

  free(packet);

  return ret;
}

DEFINE_SELFTEST() {
  // Stolen packet from wireshark
  u8 buffer[] = {
      0xc0,
      0xa8,
      0x00,
      0x1f, // Source IP: 192.168.0.31

      0xc0,
      0xa8,
      0x00,
      0x1e, // Destination IP: 192.168.0.30
      0x00,
      0x00,
      0x00,
      0x11, // Reserved/UDP Protocol
      0x00,
      0x0A, // Padding/Length

      // UDP Header
      0x00,
      0x14, // Source Port: 20
      0x00,
      0x0A, // Destination Port: 10
      0x00,
      0x0A, // Length: 10
      0x48,
      0x69, // UDP Data
  };
  u16 expected = 0xC535;
  // in real code this needs to convert from big endian to little endian.
  // excluded here because its not neccesary to test.
  assert(inet_checksum(buffer, sizeof(buffer)) == expected);
}

DEFINE_SELFTEST() {
  u8 bytes[] = {
      0x80,       // type echo request
      0x0,        // code 0
      0x0,  0x0,  // checksum
      0x82, 0x2a, // identifier
      0x0,  0x1,  // sequence
      0x90, 0xd2, 0xb6, 0x6a, 0x0,  0x0,  0x0,  0x0,  0x70, 0x9b, 0xa,  0x0,
      0x0,  0x0,  0x0,  0x0,  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
      0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 // data
  };
  struct ip_address src = {{0x26, 0x07, 0xea, 0x0, 0x1, 0x7, 0x1c, 0x7, 0x59,
                            0xc1, 0xf6, 0x2c, 0xc0, 0xee, 0x08, 0xd7}};
  struct ip_address dst = {{0x26, 0x07, 0xea, 0x0, 0x1, 0x1, 0x4, 0x21, 0x0,
                            0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
  u16 checksum = inet_checksum_with_ip_pseudo_header(
      src, dst, NET_PROTOCOL_ICMP, bytes, sizeof(bytes));
  assert(checksum == 0xb920);
}
