#ifndef UKO_OS_KERNEL__CRYPTO_SUBTLE_RANDOM_INTERNALS_H
#define UKO_OS_KERNEL__CRYPTO_SUBTLE_RANDOM_INTERNALS_H 1

#include <types.h>

struct random_rng {
  u8 state[32];
  bool inited : 1;
  u64 entropy_estimate : 63;
};

#endif // UKO_OS_KERNEL__CRYPTO_SUBTLE_RANDOM_INTERNALS_H
