/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <crypto/subtle/random_internals.h>
#include <crypto/subtle/rfc7539.h>
#include <crypto/subtle/rfc7693.h>
#include <hart_locals.h>
#include <minmax.h>
#include <panic.h>
#include <random.h>

/**
 * A random number generator based on the same approach and ciphers Linux uses.
 * The actual construction is as follows:
 *
 * - The entropy pool is simply the input to the BLAKE2s keyed hash function. As
 *   bytes are given to `entropy_pool_mix`, they are given to the hash function.
 * - Values are drawn from the pool by finalizing the hash function. The same
 *   value that is returned from the pool is used as a key to reinitialize the
 *   pool.
 * - We keep an estimate of the total entropy that has ever been added to the
 *   pool. `wait_for_entropy` waits until this reaches 256 bits; after this
 *   point, the pool has "maxed out," since BLAKE2s is a 256-bit hash.
 * - The values extracted from the entropy pool are used to seed the CSPRNGs,
 *   which are per-hart.
 * - The CSPRNGs are reseeded for every 256 bits of entropy that are added to
 *   the entropy pool.
 * - The core of the CSPRNGs is the ChaCha20 block function, applied to a state
 *   with a zero nonce. We split the output into two halves, giving us a
 *   function `F: 𝔹²⁵⁶ -> 𝔹²⁵⁶ × 𝔹²⁵⁶`, which we use to generate random values
 *   as `stateʹ, value <- F(state)`.
 * - Finally, the interface exposed as `getrandom` is the ChaCha20 keystream,
 *   using a value drawn from the hart-local CSPRNG as the key.
 */

/**
 * The global entropy pool.
 */
static struct blake2s_hasher entropy_pool = {0};

/**
 * The number of bits of entropy we believe have _ever_ been added to the pool.
 * This is used for a couple of things:
 *
 * - Users of the RNG that need to generate cryptographic keys with it can call
 *   `wait_for_entropy` to block until this reaches 256 bits. Hopefully (though
 *   we cannot depend on this), this much entropy is gathered during boot;
 *   either from timing variations on real hardware, or from hypervisor-provided
 *   sources (Devicetree and virtio) in virtual machines.
 *
 * - We use this as a kind of age counter for hart-local RNGs, and reseed them
 *   every time an additional 256 bits of entropy are gathered.
 */
static u64 global_entropy_estimate = 0;

void entropy_pool_init(void) { blake2s_init(&entropy_pool, 32, nullptr, 0); }

void entropy_pool_credit(usize bits) { global_entropy_estimate += bits; }

void entropy_pool_mix(const u8 *buf, usize len) {
  blake2s_update(&entropy_pool, buf, len);
}

/**
 * Generates a stream of random bytes from `rng`.
 */
static void rng_getrandom(struct random_rng *rng, u8 *ptr, usize len) {
  assert(rng->inited);

  union chacha20_state state;
  const u32 zero_nonce[3] = {0};
  chacha20_state_init(&state, rng->state, 0, zero_nonce);
  chacha20_block(&state);
  memcpy(rng->state, &state.words[0], 32);
  chacha20_keystream(&state.bytes[32], 0, zero_nonce, ptr, len);
  explicit_bzero(&state, sizeof(state));
}

/**
 * Returns the hart-local RNG, possibly initializing or re-initializing it if
 * necessary.
 */
static struct random_rng *ensure_hart_local_rng(void) {
  struct random_rng *rng = &get_hart_locals()->rng;

  // See if the global heap's entropy estimate is more than 256 bits "ahead" of
  // the one the local heap was initialized with, or if we never actually
  // initialized it.
  bool should_reinit =
      !rng->inited ||
      ((rng->entropy_estimate == 0) && global_entropy_estimate != 0) ||
      (global_entropy_estimate - rng->entropy_estimate > 256);

  // If the hart-local RNG needs re-initialization, do so. Otherwise, hand it
  // back.
  if (should_reinit) {
    // We use the current hash as both the next seed and the state of the RNG.
    blake2s_finish(&entropy_pool, rng->state);
    blake2s_init(&entropy_pool, 32, rng->state, 32);
    rng->inited = true;
    rng->entropy_estimate = global_entropy_estimate & 0x7fffffffffffffff;

    // TODO: Mix some extra RDSEED/Zkr entropy into the entropy pool.
  }
  return rng;
}

usize random(void) {
  u8 buf[sizeof(usize)];
  getrandom(buf, sizeof(usize));

  usize out;
  memcpy(&out, buf, sizeof(usize));
  return out;
}

void getrandom(u8 *buf, usize len) {
  rng_getrandom(ensure_hart_local_rng(), buf, len);
}
