/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <net/udp.h>
#include <panic.h>
#include <selftest.h>

[[gnu::access(read_only, 1)]]
u16 calculate_checksum(u8 *buf, usize size) {
  u32 sum = 0; // larger than u16 to catch overflow

  while (size > 1) {
    // compiler is smart about this one
    u16 tmp;
    memcpy(&tmp, buf += 2, sizeof(tmp));
    sum += tmp++;
    size -= sizeof(tmp);
  }
  if (size > 0) {
    sum += *buf; // we have only 1 byte left
  }

  // strip off overflow and wrap it around if it occured.
  sum = (sum & 0x0000FFFF) + (sum >> 16);
  sum += (sum >> 16); // previous addition can result in overflow if first bit
                      // is 1

  return (u16)~sum;
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
  assert(calculate_checksum(buffer, sizeof(buffer)) == expected);
}
