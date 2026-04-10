/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <device.h>
#include <devices/netdev.h>
#include <mm/virtual_alloc.h>
#include <net/eth.h>
#include <types.h>

struct ethernet_packet {
  u8 dst[6];
  u8 src[6];
  u8 ethertype[2];
  u8 data[];
};

bool eth_send_packet(struct netdev *device, const struct mac dst, u8 *buffer,
                     usize len) {
  struct ethernet_packet *packet;
  struct mac src;
  usize packet_len;

  packet_len = len + sizeof(struct ethernet_packet);
  if (len < 46) {
    packet_len = 46 + sizeof(struct ethernet_packet);
  }

  packet = alloc(packet_len);

  memcpy(packet->dst, dst.addr, 6);
  src = device->ops->get_mac(device);
  memcpy(packet->src, src.addr, 6);

  packet->ethertype[0] = 0x86;
  packet->ethertype[1] = 0xDD;

  memcpy(packet->data, buffer, len);

  for (usize i = len; i < 46; ++i) {
    packet->data[i] = 0;
  }

  return device->ops->send_packet(device, (u8 *)packet, packet_len);
}
