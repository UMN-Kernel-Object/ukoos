/* SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <container.h>
#include <devices/netdev.h>
#include <endian.h>
#include <net/eth.h>
#include <net/ipv6.h>

struct ip_header ip_create_header(struct ip_address src_address,
                                  struct ip_address dst_address,
                                  enum ip_next_protocol_value protocol) {
  struct ip_header header = {0};
  header.vtf[0] = 0x60;
  header.next_header = protocol;
  header.hop_limit = 64;

  memcpy(&header.src, &src_address, sizeof(header.src));
  memcpy(&header.dst, &dst_address, sizeof(header.dst));
  return header;
}

u64 ip_send_packet(struct ip_header header, u8 *data, usize len) {
  header.payload_len = native_to_big((u16)len);
  usize bufsiz = len + sizeof(struct ip_header);
  u8 *buf = alloc(bufsiz);
  if (!buf) {
    return 1;
  }

  memcpy(buf, &header, sizeof(header));
  memcpy(buf + sizeof(header), data, len);

  struct netdev *netdev = container_of(netdevs.next, struct netdev, list);
  eth_send_packet(netdev, netdev->ops->get_mac(netdev), buf, bufsiz);
  free(buf);
  return 0;
}
