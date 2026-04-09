#ifndef UKO_OS__NET_ICMPV6_H
#define UKO_OS__NET_ICMPV6_H

#include <net/ipv6.h>
#include <types.h>

enum icmpv6_type {
  ICMPV6_DST_UNREACHABLE = 1,
  ICMPV6_PACKET_TOO_BIG,
  ICMPV6_TIME_EXCEEDED,
  ICMPV6_PARAM_PROBLEM,

  ICMPV6_ECHO_REQUEST = 128,
  ICMPV6_ECHO_REPLY,

  NDP_ROUTER_SOLICIT = 133,
  NDP_ROUTER_ADVERT,
  NDP_NEIGHBOR_SOLICIT,
  NDP_NEIGHBOR_ADVERT,
  NDP_REDIRECT,
};

struct icmpv6_header {
  u8 type;
  u8 code;
  u16 cksum;
};

static_assert(sizeof(struct icmpv6_header) == 4);

struct icmpv6_message {
  struct icmpv6_header header;
  union {
    // packet too big
    u32 mtu;

    // parameter problem
    u32 offset;

    // echo request and reply
    struct {
      u16 ident;
      u16 sequence_num;
    };

    // unused otherwise
  };
};

static_assert(sizeof(struct icmpv6_message) == 8);

struct ndp_router_solicit {
  struct icmpv6_header header;
  u8 reserved[4];
};

static_assert(sizeof(struct ndp_router_solicit) == 8);

struct ndp_router_advert {
  struct icmpv6_header header;
  u8 cur_hop_limit;
  u8 flags;
  u16 lifetime;
  u32 reachable_time;
  u32 retrans_timer;
};

static_assert(sizeof(struct ndp_router_advert) == 16);

struct ndp_neighbor_solicit {
  struct icmpv6_header header;
  u8 flags;
  u8 reserved[3];
  u8 target_addr[16];
};

static_assert(sizeof(struct ndp_neighbor_solicit) == 24);

struct ndp_redirect {
  struct icmpv6_header header;
  u8 reserved[4];
  u8 target_addr[16];
  u8 dst_addr[16];
};

static_assert(sizeof(struct ndp_redirect) == 40);

struct ndp_option {
  u8 type;
  u8 length;
};

struct icmpv6_header icmpv6_create_header(struct ip_address src,
                                          struct ip_address dst, u8 *data,
                                          usize len, u8 type, u8 code);

#endif // UKO_OS__NET_ICMPV6_H
