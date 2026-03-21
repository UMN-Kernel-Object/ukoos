/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <crypto/subtle/rfc7693.h>
#include <endian.h>
#include <minmax.h>
#include <panic.h>
#include <selftest.h>

/**
 * The shared permutations between BLAKE2b and BLAKE2s. BLAKE2s only uses the
 * first 10 rows.
 */
static const u8 blake2_sigma[12][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
    {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
    {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
    {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
    {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
    {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
    {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
    {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
    {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}};

static const u64 blake2b_iv[8] = {0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
                                  0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
                                  0x510e527fade682d1, 0x9b05688c2b3e6c1f,
                                  0x1f83d9abfb41bd6b, 0x5be0cd19137e2179};

static const u32 blake2s_iv[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372,
                                  0xa54ff53a, 0x510e527f, 0x9b05688c,
                                  0x1f83d9ab, 0x5be0cd19};

/**
 * Compare to `chacha20_quarter_round`. Unlike that function:
 *
 * - This uses right rotations instead of left (due to a typo in the original
 *   BLAKE specification).
 * - This mixes `x` and `y` into `words` (to mix the secret data in, in a way
 *   that's less prone to an attacker with knowledge of the state controlling
 *   the state by controlling the plaintext).
 */
static void blake2s_mix_quarter_round(u32 words[static 16], usize a, usize b,
                                      usize c, usize d, u32 x, u32 y) {
  words[a] += words[b] + x;
  words[d] = stdc_rotate_right(words[d] ^ words[a], 16);
  words[c] += words[d];
  words[b] = stdc_rotate_right(words[b] ^ words[c], 12);
  words[a] += words[b] + y;
  words[d] = stdc_rotate_right(words[d] ^ words[a], 8);
  words[c] += words[d];
  words[b] = stdc_rotate_right(words[b] ^ words[c], 7);
}

/**
 * Compare to `chacha20_double_round`. Unlike that function, this mixes in
 * plaintext words according to the permutation schedule `sigma`.
 */
static void blake2s_mix_double_round(u32 words[static 16],
                                     const u32 chunk[static 16], usize i) {
  const u8 *s = blake2_sigma[i];
  blake2s_mix_quarter_round(words, 0, 4, 8, 12, chunk[s[0]], chunk[s[1]]);
  blake2s_mix_quarter_round(words, 1, 5, 9, 13, chunk[s[2]], chunk[s[3]]);
  blake2s_mix_quarter_round(words, 2, 6, 10, 14, chunk[s[4]], chunk[s[5]]);
  blake2s_mix_quarter_round(words, 3, 7, 11, 15, chunk[s[6]], chunk[s[7]]);
  blake2s_mix_quarter_round(words, 0, 5, 10, 15, chunk[s[8]], chunk[s[9]]);
  blake2s_mix_quarter_round(words, 1, 6, 11, 12, chunk[s[10]], chunk[s[11]]);
  blake2s_mix_quarter_round(words, 2, 7, 8, 13, chunk[s[12]], chunk[s[13]]);
  blake2s_mix_quarter_round(words, 3, 4, 9, 14, chunk[s[14]], chunk[s[15]]);
}

static void blake2s_compress(u32 state[static 8], u32 chunk[static 16],
                             u32 byte_count_lo, u32 byte_count_hi, bool final) {
  u32 words[16];

  for (usize i = 0; i < 8; i++)
    words[i] = state[i];
  for (usize i = 0; i < 8; i++)
    words[8 + i] = blake2s_iv[i];
  words[12] ^= byte_count_lo;
  words[13] ^= byte_count_hi;
  if (final)
    words[14] = ~words[14];

  for (usize i = 0; i < 10; i++)
    blake2s_mix_double_round(words, chunk, i);

  for (usize i = 0; i < 8; i++)
    state[i] ^= words[i];
  for (usize i = 0; i < 8; i++)
    state[i] ^= words[i + 8];

  explicit_bzero(words, 64);
}

void blake2s_init(struct blake2s_hasher *hasher, usize hash_len, const u8 *key,
                  usize key_len) {
  assert(1 <= hash_len && hash_len <= 32);
  assert(0 <= key_len && key_len <= 32);

  // Initialize the state.
  for (usize i = 0; i < 8; i++)
    hasher->hash_state[i] = blake2s_iv[i];
  hasher->hash_state[0] ^= 0x01010000 ^ ((u32)key_len << 8) ^ (u32)hash_len;
  hasher->byte_count_lo = 0;
  hasher->byte_count_hi = 0;
  hasher->buffer_len = 0;
  hasher->hash_len = (u8)hash_len;

  // If there was a key, process it.
  if (key_len) {
    u8 key_buf[64] = {0};
    memcpy(key_buf, key, key_len);
    blake2s_update(hasher, key_buf, 64);
    assert(hasher->byte_count_lo == 0);
    assert(hasher->byte_count_hi == 0);
    assert(hasher->buffer_len == 64);
  }
}

void blake2s_update(struct blake2s_hasher *hasher, const u8 *data,
                    usize data_len) {
  assert(hasher->buffer_len <= 64);

  while (data_len > 0) {
    // If the buffer is full, compress it. We do it just before adding another
    // byte, rather than just after adding a 64th, so we know what the `final`
    // flag is.
    if (hasher->buffer_len == 64) {
      u32 buffer[16];
      memcpy(buffer, hasher->buffer, 64);
      for (usize i = 0; i < 16; i++)
        buffer[i] = little_to_native(buffer[i]);
      if (ckd_add(&hasher->byte_count_lo, hasher->byte_count_lo, 64))
        assert(!ckd_add(&hasher->byte_count_hi, hasher->byte_count_hi, 1));
      blake2s_compress(hasher->hash_state, buffer, hasher->byte_count_lo,
                       hasher->byte_count_hi, false);
      hasher->buffer_len = 0;
    }

    // Copy some bytes into the buffer; enough that either we've finished or
    // we've filled the buffer.
    usize chunk_len = min(data_len, (usize)64 - hasher->buffer_len);
    memcpy(&hasher->buffer[hasher->buffer_len], data, chunk_len);
    hasher->buffer_len += (u8)chunk_len;
    data += chunk_len;
    data_len -= chunk_len;
  }
}

void blake2s_finish(const struct blake2s_hasher *hasher, u8 *out) {
  assert(hasher->buffer_len <= 64);

  // Copy the state from the hasher.
  u32 state[8];
  memcpy(state, hasher->hash_state, 32);

  // Copy the bytes still in the buffer.
  u32 buffer[16] = {0};
  memcpy(buffer, hasher->buffer, hasher->buffer_len);
  for (usize i = 0; i < 16; i++)
    buffer[i] = little_to_native(buffer[i]);

  // Update the length for the bytes still in the buffer.
  u32 byte_count_lo = hasher->byte_count_lo,
      byte_count_hi = hasher->byte_count_hi;
  if (ckd_add(&byte_count_lo, byte_count_lo, hasher->buffer_len))
    assert(!ckd_add(&byte_count_hi, byte_count_hi, 1));

  // Compress the final block.
  blake2s_compress(state, buffer, byte_count_lo, byte_count_hi, true);

  // Convert state to little-endian and copy it out.
  for (usize i = 0; i < 8; i++)
    state[i] = native_to_little(state[i]);
  memcpy(out, state, hasher->hash_len);

  // Clear state left on the stack.
  explicit_bzero(state, 32);
  explicit_bzero(buffer, 64);
}

void blake2s_hash(u8 *out_hash, usize hash_len, const u8 *key, usize key_len,
                  const u8 *data, usize data_len) {
  struct blake2s_hasher hasher;
  blake2s_init(&hasher, hash_len, key, key_len);
  blake2s_update(&hasher, data, data_len);
  blake2s_finish(&hasher, out_hash);
}

static void blake2b_mix_quarter_round(u64 words[static 16], usize a, usize b,
                                      usize c, usize d, u64 x, u64 y) {
  words[a] += words[b] + x;
  words[d] = stdc_rotate_right(words[d] ^ words[a], 32);
  words[c] += words[d];
  words[b] = stdc_rotate_right(words[b] ^ words[c], 24);
  words[a] += words[b] + y;
  words[d] = stdc_rotate_right(words[d] ^ words[a], 16);
  words[c] += words[d];
  words[b] = stdc_rotate_right(words[b] ^ words[c], 63);
}

static void blake2b_mix_double_round(u64 words[static 16],
                                     const u64 chunk[static 16], usize i) {
  const u8 *s = blake2_sigma[i];
  blake2b_mix_quarter_round(words, 0, 4, 8, 12, chunk[s[0]], chunk[s[1]]);
  blake2b_mix_quarter_round(words, 1, 5, 9, 13, chunk[s[2]], chunk[s[3]]);
  blake2b_mix_quarter_round(words, 2, 6, 10, 14, chunk[s[4]], chunk[s[5]]);
  blake2b_mix_quarter_round(words, 3, 7, 11, 15, chunk[s[6]], chunk[s[7]]);
  blake2b_mix_quarter_round(words, 0, 5, 10, 15, chunk[s[8]], chunk[s[9]]);
  blake2b_mix_quarter_round(words, 1, 6, 11, 12, chunk[s[10]], chunk[s[11]]);
  blake2b_mix_quarter_round(words, 2, 7, 8, 13, chunk[s[12]], chunk[s[13]]);
  blake2b_mix_quarter_round(words, 3, 4, 9, 14, chunk[s[14]], chunk[s[15]]);
}

static void blake2b_compress(u64 state[static 8], u64 chunk[static 16],
                             u64 byte_count_lo, u64 byte_count_hi, bool final) {
  u64 words[16];

  for (usize i = 0; i < 8; i++)
    words[i] = state[i];
  for (usize i = 0; i < 8; i++)
    words[8 + i] = blake2b_iv[i];
  words[12] ^= byte_count_lo;
  words[13] ^= byte_count_hi;
  if (final)
    words[14] = ~words[14];

  for (usize i = 0; i < 12; i++)
    blake2b_mix_double_round(words, chunk, i);

  for (usize i = 0; i < 8; i++)
    state[i] ^= words[i];
  for (usize i = 0; i < 8; i++)
    state[i] ^= words[i + 8];

  explicit_bzero(words, 128);
}

void blake2b_hash(u8 *out_hash, usize hash_len, const u8 *key, usize key_len,
                  const u8 *data, usize data_len) {
  u64 state[8];
  u64 chunk[16];
  u64 byte_count_lo = 0, byte_count_hi = 0;

  assert(1 <= hash_len && hash_len <= 64);
  assert(0 <= key_len && key_len <= 64);
  assert(0 <= data_len && data_len <= U64_MAX);

  // Initialize the state.
  for (usize i = 0; i < 8; i++)
    state[i] = blake2b_iv[i];
  state[0] ^= 0x01010000 ^ ((u32)key_len << 8) ^ (u32)hash_len;

  // If there was a key, it fits in (half of!) one block, so we can just handle
  // it as a single block.
  if (key_len > 0) {
    bzero(chunk, 128);
    memcpy(chunk, key, min(key_len, (usize)128));
    for (usize i = 0; i < 16; i++)
      chunk[i] = little_to_native(chunk[i]);
    if (ckd_add(&byte_count_lo, byte_count_lo, 128))
      byte_count_hi++;
    blake2b_compress(state, chunk, byte_count_lo, byte_count_hi, data_len == 0);
  }

  // If there was neither a key nor a data block, we still hash a block.
  if (key_len == 0 && data_len == 0) {
    bzero(chunk, 128);
    blake2b_compress(state, chunk, 0, 0, true);
  }

  // Process data blocks.
  while (data_len > 0) {
    usize chunk_len = min(data_len, (usize)128);
    bzero(chunk, 128);
    memcpy(chunk, data, chunk_len);
    for (usize i = 0; i < 16; i++)
      chunk[i] = little_to_native(chunk[i]);
    if (ckd_add(&byte_count_lo, byte_count_lo, chunk_len))
      byte_count_hi++;
    blake2b_compress(state, chunk, byte_count_lo, byte_count_hi,
                     data_len == chunk_len);
    data += chunk_len;
    data_len -= chunk_len;
  }

  // Convert state to little-endian and copy it to out_hash.
  for (usize i = 0; i < 8; i++)
    state[i] = native_to_little(state[i]);
  memcpy(out_hash, state, hash_len);

  // Clear the state, to mitigate arbitrary read vulnerabilities stealing
  // secrets off the stack.
  explicit_bzero(state, 32);
  explicit_bzero(chunk, 64);
}

// Appendix A of RFC7693.
DEFINE_SELFTEST() {
  u8 hash[64];
  blake2b_hash(hash, 64, nullptr, 0, (const u8 *)"abc", 3);

  const u8 expected[64] = {
      0xba, 0x80, 0xa5, 0x3f, 0x98, 0x1c, 0x4d, 0x0d, 0x6a, 0x27, 0x97,
      0xb6, 0x9f, 0x12, 0xf6, 0xe9, 0x4c, 0x21, 0x2f, 0x14, 0x68, 0x5a,
      0xc4, 0xb7, 0x4b, 0x12, 0xbb, 0x6f, 0xdb, 0xff, 0xa2, 0xd1, 0x7d,
      0x87, 0xc5, 0x39, 0x2a, 0xab, 0x79, 0x2d, 0xc2, 0x52, 0xd5, 0xde,
      0x45, 0x33, 0xcc, 0x95, 0x18, 0xd3, 0x8a, 0xa8, 0xdb, 0xf1, 0x92,
      0x5a, 0xb9, 0x23, 0x86, 0xed, 0xd4, 0x00, 0x99, 0x23};
  for (usize i = 0; i < 64; i++)
    assert(hash[i] == expected[i], "{u8:#04x} == {u8:#04x} (at index {usize})",
           hash[i], expected[i], i);
}

// Appendix B of RFC7693.
DEFINE_SELFTEST() {
  u8 hash[32];
  blake2s_hash(hash, 32, nullptr, 0, (const u8 *)"abc", 3);

  const u8 expected[32] = {0x50, 0x8c, 0x5e, 0x8c, 0x32, 0x7c, 0x14, 0xe2,
                           0xe1, 0xa7, 0x2b, 0xa3, 0x4e, 0xeb, 0x45, 0x2f,
                           0x37, 0x45, 0x8b, 0x20, 0x9e, 0xd6, 0x3a, 0x29,
                           0x4d, 0x99, 0x9b, 0x4c, 0x86, 0x67, 0x59, 0x82};
  for (usize i = 0; i < 32; i++)
    assert(hash[i] == expected[i], "{u8:#04x} == {u8:#04x} (at index {usize})",
           hash[i], expected[i], i);
}

static void selftest_seq(u8 *out, usize len, u32 seed) {
  u32 a = 0xdead4bad * seed;
  u32 b = 1;
  for (usize i = 0; i < len; i++) {
    u32 t = a + b;
    a = b;
    b = t;
    out[i] = (u8)(t >> 24);
  }
}

// The BLAKE2b test from Appendix E of RFC7693.
DEFINE_SELFTEST() {
  const u32 hash_lens[4] = {20, 32, 48, 64};
  const u32 data_lens[6] = {0, 3, 128, 129, 255, 1024};
  // TODO: Once we give the boothart a larger stack (and move the selftests to
  // after that point), make these not static.
  static u8 data[1024], hashes[1968];
  u8 hash[64], key[64], *next_hash = hashes;
  for (usize i = 0; i < 4; i++) {
    const u32 hash_len = hash_lens[i];
    for (usize j = 0; j < 6; j++) {
      const u32 data_len = data_lens[j];

      selftest_seq(data, data_len, data_len);
      blake2b_hash(hash, hash_len, nullptr, 0, data, data_len);
      memcpy(next_hash, hash, hash_len);
      next_hash += hash_len;

      selftest_seq(key, hash_len, hash_len);
      blake2b_hash(hash, hash_len, key, hash_len, data, data_len);
      memcpy(next_hash, hash, hash_len);
      next_hash += hash_len;
    }
  }
  assert(next_hash == hashes + sizeof(hashes));
  blake2b_hash(hash, 32, nullptr, 0, hashes, (usize)(next_hash - hashes));

  const u8 expected[32] = {0xc2, 0x3a, 0x78, 0x00, 0xd9, 0x81, 0x23, 0xbd,
                           0x10, 0xf5, 0x06, 0xc6, 0x1e, 0x29, 0xda, 0x56,
                           0x03, 0xd7, 0x63, 0xb8, 0xbb, 0xad, 0x2e, 0x73,
                           0x7f, 0x5e, 0x76, 0x5a, 0x7b, 0xcc, 0xd4, 0x75};
  for (usize i = 0; i < 32; i++)
    assert(hash[i] == expected[i], "{u8:#04x} == {u8:#04x} (at index {usize})",
           hash[i], expected[i], i);
}

// The BLAKE2s test from Appendix E of RFC7693.
DEFINE_SELFTEST() {
  const u32 hash_lens[4] = {16, 20, 28, 32};
  const u32 data_lens[6] = {0, 3, 64, 65, 255, 1024};
  // TODO: Once we give the boothart a larger stack (and move the selftests to
  // after that point), make these not static.
  static u8 data[1024], hashes[1152];
  u8 hash[32], key[32], *next_hash = hashes;
  for (usize i = 0; i < 4; i++) {
    const u32 hash_len = hash_lens[i];
    for (usize j = 0; j < 6; j++) {
      const u32 data_len = data_lens[j];

      selftest_seq(data, data_len, data_len);
      blake2s_hash(hash, hash_len, nullptr, 0, data, data_len);
      memcpy(next_hash, hash, hash_len);
      next_hash += hash_len;

      selftest_seq(key, hash_len, hash_len);
      blake2s_hash(hash, hash_len, key, hash_len, data, data_len);
      memcpy(next_hash, hash, hash_len);
      next_hash += hash_len;
    }
  }
  assert(next_hash == hashes + sizeof(hashes));
  blake2s_hash(hash, 32, nullptr, 0, hashes, (usize)(next_hash - hashes));

  const u8 expected[32] = {0x6a, 0x41, 0x1f, 0x08, 0xce, 0x25, 0xad, 0xcd,
                           0xfb, 0x02, 0xab, 0xa6, 0x41, 0x45, 0x1c, 0xec,
                           0x53, 0xc5, 0x98, 0xb2, 0x4f, 0x4f, 0xc7, 0x87,
                           0xfb, 0xdc, 0x88, 0x79, 0x7f, 0x4c, 0x1d, 0xfe};
  for (usize i = 0; i < 32; i++)
    assert(hash[i] == expected[i], "{u8:#04x} == {u8:#04x} (at index {usize})",
           hash[i], expected[i], i);
}
