#include <types.h>
#include <device.h>
#include <devices/netdev.h>
#include <mm/virtual_alloc.h>
#include <net/eth.h>

struct ethernet_packet {
    u8 dst[6];
    u8 src[6];
    u8 ethertype[2];
    u8 data[];
};

bool eth_send_packet(struct netdev *device, const u8 *dstmac, u8 *buffer, u32 len) {
  struct ethernet_packet *packet;
  usize packet_len;

  packet_len = len + sizeof(struct ethernet_packet);
  if (len < 46) {
    packet_len = 46 + sizeof(struct ethernet_packet);
  }

  packet = alloc(packet_len);

  memcpy(packet->dst, dstmac, 6);
  device->ops->get_mac(device, packet->src);

  packet->ethertype[0] = 0x08;
  packet->ethertype[1] = 0x00;
  
  memcpy(packet->data, buffer, len);

  for (usize i=len; i<46; ++i) {
    packet->data[i] = 0;
  }

  return device->ops->send_packet(device, (u8*) packet, packet_len);
}
