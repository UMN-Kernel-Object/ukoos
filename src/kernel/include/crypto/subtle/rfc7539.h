#ifndef UKO_OS_KERNEL__CRYPTO_SUBTLE_RFC7539_H
#define UKO_OS_KERNEL__CRYPTO_SUBTLE_RFC7539_H 1

#include <types.h>

/**
 * The ChaCha20 internal state.
 */
union chacha20_state {
  u32 words[16];
  struct {
    u32 constant[4];
    u32 key[8];
    u32 block_count;
    u32 nonce[3];
  };
};

/**
 * Performs the ChaCha20 block function in place.
 *
 * This is a "subtle" cryptographic function. Do not home-brew a cryptographic
 * construction using it.
 */
void chacha20_block(union chacha20_state *state);

/**
 * Encrypts or decrypts the buffer given by `ptr` and `len` in place using
 * `key`, `block_count`, and `nonce`.
 *
 * This is a "subtle" cryptographic function. Do not home-brew a cryptographic
 * construction using it.
 */
__attribute__((access(read_only, 4, 5))) void
chacha20_encrypt(const u8 key[static 32], u32 block_count,
                 const u32 nonce[static 3], void *ptr, usize len);

// TODO: Sections 2.5-2.8

#endif // UKO_OS_KERNEL__CRYPTO_SUBTLE_RFC7539_H
