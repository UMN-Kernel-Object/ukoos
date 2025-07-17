#ifndef UKO_OS_KERNEL__CRYPTO_SUBTLE_RFC7693_H
#define UKO_OS_KERNEL__CRYPTO_SUBTLE_RFC7693_H 1

#include <types.h>

/**
 * The BLAKE2s hash function.
 *
 * This is less subtle than other primitives when used as a simple check for
 * e.g. file integrity, but subtleties arise when using it to check message
 * integrity. Do not home-brew a cryptographic construction using it.
 */
__attribute__((access(write_only, 1, 2), access(read_only, 3, 4),
               access(read_only, 5, 6))) void
blake2s_hash(u8 *out_hash, usize hash_len, const u8 *key, usize key_len,
             const u8 *data, usize data_len);

/**
 * The BLAKE2b hash function.
 *
 * This is less subtle than other primitives when used as a simple check for
 * e.g. file integrity, but subtleties arise when using it to check message
 * integrity. Do not home-brew a cryptographic construction using it.
 */
__attribute__((access(write_only, 1, 2), access(read_only, 3, 4),
               access(read_only, 5, 6))) void
blake2b_hash(u8 *out_hash, usize hash_len, const u8 *key, usize key_len,
             const u8 *data, usize data_len);

#endif // UKO_OS_KERNEL__CRYPTO_SUBTLE_RFC7693_H
