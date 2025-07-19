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

/**
 * A struct that bytes can be added to, in order to hash them.
 */
struct blake2s_hasher {
  u8 buffer[64];
  u32 hash_state[8];
  u32 byte_count_lo, byte_count_hi;
  u8 buffer_len;
  u8 hash_len;
};

/**
 * Initializes the `blake2s_hasher` for the given `hash_len`.
 */
__attribute__((access(write_only, 1), access(read_only, 3, 4))) void
blake2s_init(struct blake2s_hasher *hasher, usize hash_len, const u8 *key,
             usize key_len);

/**
 * Adds bytes to the `blake2s_hasher`.
 */
__attribute__((access(read_write, 1), access(read_only, 2, 3))) void
blake2s_update(struct blake2s_hasher *hasher, const u8 *data, usize data_len);

/**
 * Finalizes the hash, writing it out to `out`.
 */
__attribute__((access(read_only, 1), access(write_only, 2))) void
blake2s_finish(const struct blake2s_hasher *hasher, u8 *out);

#endif // UKO_OS_KERNEL__CRYPTO_SUBTLE_RFC7693_H
