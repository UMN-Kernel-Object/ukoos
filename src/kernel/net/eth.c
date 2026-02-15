#include <types.h>
#include <device.h>
#include <devices/netdev.h>

bool eth_send_packet(struct netdev *device, u8 *buffer, u32 len) {
  u8 buf[0x100];
  u64 i;

  for (i=0; i<6; ++i) {
    buf[i] = 0xff;
  }

  print("getting mac");
  device->ops->get_mac(device, buf+6);

  buf[12] = 0x08;
  buf[13] = 0x00;

  for (i=0; i<len; ++i) {
    buf[i + 14] = buffer[i];
  }
  for (i=len; i<64; ++i) {
    buf[i + 14] = 0;
  }
  if (len < 64) len = 64;

  print("sending");
  return device->ops->send_packet(device, buf, len + 13);
}
