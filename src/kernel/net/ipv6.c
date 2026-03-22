#include <net/ipv6.h>
#include <net/eth.h>
#include <devices/netdev.h>
#include <container.h>
#include <endian.h>

struct ipv6_header ipv6_create_header(u8 src_address[static 16],
                                      u8 dst_address[static 16], u8 protocol) {
	struct ipv6_header header = {0};
	header.vtf[0] = 0x60;
	header.next_header = protocol;
	header.hop_limit = 64;


	memcpy(header.src, src_address, 16 * sizeof(u8));
	memcpy(header.dst, dst_address, 16 * sizeof(u8));
	return header;
}

long ipv6_send_packet(struct ipv6_header header, u8 *data, usize len) {
  header.payload_len = native_to_big( (u16) len);
  usize bufsiz = len + sizeof(struct ipv6_header);
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
