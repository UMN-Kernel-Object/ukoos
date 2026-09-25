#include <net/common.h>
#include <endian.h>

struct ip_pseudo_header {
  struct ip_address src;
  struct ip_address dst;
  u32 upper_layer_len;
  u8 resvd[3];
  u8 next_header;
};

static u16 get_u16(u8 *buf) {
  u16 n = 0;
  memcpy(&n, buf, sizeof(n));
  return n;
}

static u16 inet_checksum_with_offset(u32 offset, u8 *data, usize count) {
  u32 sum = offset;
  usize i = 0;

  for (; i < count - (count % 2); i += sizeof(u16)) {
    sum += (u32)get_u16(data + i);
  }

  if (i < count) {
    sum += (u32)data[i];
  }

  while (sum >> 16 != 0) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  return (u16)~sum;
}

u16 inet_checksum(u8 *data, usize count) {
  return inet_checksum_with_offset(0, data, count);
}

u16 inet_checksum_with_ip_pseudo_header(struct ip_address src,
                                        struct ip_address dst, u8 ip_proto_num,
                                        u8 *data, usize data_len) {
  u32 pseudo_header_checksum = 0;
  struct ip_pseudo_header pseudo_header = {
      .src = src,
      .dst = dst,
      .upper_layer_len = native_to_big((u32)data_len),
      .resvd = {0},
      .next_header = ip_proto_num,
  };

  for (usize i = 0; i < sizeof(pseudo_header); i += sizeof(u16)) {
    pseudo_header_checksum += (u32)get_u16((u8 *)&pseudo_header + i);
  }

  return inet_checksum_with_offset(pseudo_header_checksum, data, data_len);
}

