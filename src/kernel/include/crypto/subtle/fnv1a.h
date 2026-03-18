/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__CRYPTO_SUBTLE_FNV1A_H
#define UKO_OS_KERNEL__CRYPTO_SUBTLE_FNV1A_H 1

#include <types.h>

/**
 * A hasher for the FNV-1a hash.
 */
struct fnv1a_hasher {
  u64 hash;
};

constexpr u64 FNV1A_OFFSET_BASIS = 0xcbf29ce484222325;
constexpr u64 FNV1A_PRIME = 0x00000100000001b3;

static inline void fnv1a_hash_bytes(struct fnv1a_hasher *hasher,
                                    const void *ptr, usize len) {
  while (len--) {
    u8 byte = *(u8 *)ptr++;
    hasher->hash = (hasher->hash ^ (u64)byte) * FNV1A_PRIME;
  }
}

#define fnv1a_hash(HASHER, VALUE)                                              \
  ({                                                                           \
    struct fnv1a_hasher *__fnv1a_hash_HASHER = (HASHER);                       \
    auto __fnv1a_hash_VALUE = (VALUE);                                         \
    fnv1a_hash_bytes(__fnv1a_hash_HASHER, &__fnv1a_hash_VALUE,                 \
                     sizeof(__fnv1a_hash_VALUE));                              \
  })

#endif // UKO_OS_KERNEL__CRYPTO_SUBTLE_FNV1A_H
