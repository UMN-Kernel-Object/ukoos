#include <crypto/subtle/rfc7539.h>
#include <endian.h>
#include <minmax.h>
#include <panic.h>
#include <selftest.h>

static void chacha20_quarter_round(u32 words[static 16], usize a, usize b,
                                   usize c, usize d) {
  words[a] += words[b];
  words[d] = stdc_rotate_left(words[d] ^ words[a], 16);
  words[c] += words[d];
  words[b] = stdc_rotate_left(words[b] ^ words[c], 12);
  words[a] += words[b];
  words[d] = stdc_rotate_left(words[d] ^ words[a], 8);
  words[c] += words[d];
  words[b] = stdc_rotate_left(words[b] ^ words[c], 7);
}

static void chacha20_double_round(union chacha20_state *state) {
  chacha20_quarter_round(state->words, 0, 4, 8, 12);
  chacha20_quarter_round(state->words, 1, 5, 9, 13);
  chacha20_quarter_round(state->words, 2, 6, 10, 14);
  chacha20_quarter_round(state->words, 3, 7, 11, 15);
  chacha20_quarter_round(state->words, 0, 5, 10, 15);
  chacha20_quarter_round(state->words, 1, 6, 11, 12);
  chacha20_quarter_round(state->words, 2, 7, 8, 13);
  chacha20_quarter_round(state->words, 3, 4, 9, 14);
}

void chacha20_state_init(union chacha20_state *state, const u8 key[static 32],
                         u32 block_count, const u32 nonce[static 3]) {
  state->constant[0] = 0x61707865;
  state->constant[1] = 0x3320646e;
  state->constant[2] = 0x79622d32;
  state->constant[3] = 0x6b206574;
  for (usize i = 0; i < 8; i++) {
    u32 key_word;
    memcpy(&key_word, &key[i * 4], 4);
    state->key[i] = little_to_native(key_word);
  }
  state->block_count = block_count;
  memcpy(state->nonce, nonce, 12);
}

void chacha20_block(union chacha20_state *state) {
  union chacha20_state working_state;
  memcpy(working_state.words, state->words, sizeof(working_state.words));
  for (usize i = 0; i < 10; i++)
    chacha20_double_round(&working_state);
  for (usize i = 0; i < 16; i++)
    state->words[i] += working_state.words[i];
}

void chacha20_keystream(const u8 key[static 32], u32 block_count,
                        const u32 nonce[static 3], void *ptr, usize len) {
  union chacha20_state state;

  while (len) {
    chacha20_state_init(&state, key, block_count++, nonce);
    chacha20_block(&state);

    usize chunk_len = min(len, (usize)64);
    for (usize i = 0; i < 16; i++)
      state.words[i] = native_to_little(state.words[i]);
    memcpy(ptr, state.words, chunk_len);
    ptr += chunk_len;
    len -= chunk_len;
  }

  explicit_bzero(state.words, 64);
}

void chacha20_encrypt(const u8 key[static 32], u32 block_count,
                      const u32 nonce[static 3], void *ptr, usize len) {
  union chacha20_state state;
  u32 chunk[16];

  while (len) {
    chacha20_state_init(&state, key, block_count++, nonce);
    chacha20_block(&state);

    bzero(chunk, 64);
    usize chunk_len = min(len, (usize)64);
    memcpy(chunk, ptr, chunk_len);
    for (usize i = 0; i < 16; i++)
      chunk[i] ^= native_to_little(state.words[i]);
    memcpy(ptr, chunk, chunk_len);
    ptr += chunk_len;
    len -= chunk_len;
  }

  explicit_bzero(state.words, 64);
  explicit_bzero(chunk, 64);
}

// Section 2.1.1 of RFC7539.
DEFINE_SELFTEST() {
  u32 words[16] = {0x11111111, 0x01020304, 0x9b8d6f43, 0x01234567};
  chacha20_quarter_round(words, 0, 1, 2, 3);
  assert(words[0] == 0xea2a92f4);
  assert(words[1] == 0xcb1cf8ce);
  assert(words[2] == 0x4581472e);
  assert(words[3] == 0x5881c4bb);
}

// Section 2.2.1 of RFC7539.
DEFINE_SELFTEST() {
  u32 words[16] = {
      0x879531e0, 0xc5ecf37d, 0x516461b1, 0xc9a62f8a, 0x44c20ef3, 0x3390af7f,
      0xd9fc690b, 0x2a5f714c, 0x53372767, 0xb00a5631, 0x974c541a, 0x359e9963,
      0x5c971061, 0x3d631689, 0x2098d9d6, 0x91dbd320,
  };
  chacha20_quarter_round(words, 2, 7, 8, 13);
  assert(words[0] == 0x879531e0);
  assert(words[1] == 0xc5ecf37d);
  assert(words[2] == 0xbdb886dc);
  assert(words[3] == 0xc9a62f8a);
  assert(words[4] == 0x44c20ef3);
  assert(words[5] == 0x3390af7f);
  assert(words[6] == 0xd9fc690b);
  assert(words[7] == 0xcfacafd2);
  assert(words[8] == 0xe46bea80);
  assert(words[9] == 0xb00a5631);
  assert(words[10] == 0x974c541a);
  assert(words[11] == 0x359e9963);
  assert(words[12] == 0x5c971061);
  assert(words[13] == 0xccc07c79);
  assert(words[14] == 0x2098d9d6);
  assert(words[15] == 0x91dbd320);
}

// Section 2.3.2 of RFC7539.
DEFINE_SELFTEST() {
  union chacha20_state state;

  const u8 key[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  const u32 nonce[3] = {0x09000000, 0x4a000000, 0x00000000};
  chacha20_state_init(&state, key, 1, nonce);

  assert(state.words[0] == 0x61707865);
  assert(state.words[1] == 0x3320646e);
  assert(state.words[2] == 0x79622d32);
  assert(state.words[3] == 0x6b206574);
  assert(state.words[4] == 0x03020100);
  assert(state.words[5] == 0x07060504);
  assert(state.words[6] == 0x0b0a0908);
  assert(state.words[7] == 0x0f0e0d0c);
  assert(state.words[8] == 0x13121110);
  assert(state.words[9] == 0x17161514);
  assert(state.words[10] == 0x1b1a1918);
  assert(state.words[11] == 0x1f1e1d1c);
  assert(state.words[12] == 0x00000001);
  assert(state.words[13] == 0x09000000);
  assert(state.words[14] == 0x4a000000);
  assert(state.words[15] == 0x00000000);

  chacha20_block(&state);

  assert(state.words[0] == 0xe4e7f110);
  assert(state.words[1] == 0x15593bd1);
  assert(state.words[2] == 0x1fdd0f50);
  assert(state.words[3] == 0xc47120a3);
  assert(state.words[4] == 0xc7f4d1c7);
  assert(state.words[5] == 0x0368c033);
  assert(state.words[6] == 0x9aaa2204);
  assert(state.words[7] == 0x4e6cd4c3);
  assert(state.words[8] == 0x466482d2);
  assert(state.words[9] == 0x09aa9f07);
  assert(state.words[10] == 0x05d7c214);
  assert(state.words[11] == 0xa2028bd9);
  assert(state.words[12] == 0xd19c12b5);
  assert(state.words[13] == 0xb94e16de);
  assert(state.words[14] == 0xe883d0cb);
  assert(state.words[15] == 0x4e3c50a2);
}

// Section 2.4.2 of RFC7539.
DEFINE_SELFTEST() {
  const u8 key[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  const u32 nonce[3] = {0x00000000, 0x4a000000, 0x00000000};

  u8 text[114] = "Ladies and Gentlemen of the class of '99: If I could offer "
                 "you only one tip for the future, sunscreen would be it.";
  chacha20_encrypt(key, 1, nonce, text, 114);

  const u8 expected[114] = {
      0x6e, 0x2e, 0x35, 0x9a, 0x25, 0x68, 0xf9, 0x80, 0x41, 0xba, 0x07, 0x28,
      0xdd, 0x0d, 0x69, 0x81, 0xe9, 0x7e, 0x7a, 0xec, 0x1d, 0x43, 0x60, 0xc2,
      0x0a, 0x27, 0xaf, 0xcc, 0xfd, 0x9f, 0xae, 0x0b, 0xf9, 0x1b, 0x65, 0xc5,
      0x52, 0x47, 0x33, 0xab, 0x8f, 0x59, 0x3d, 0xab, 0xcd, 0x62, 0xb3, 0x57,
      0x16, 0x39, 0xd6, 0x24, 0xe6, 0x51, 0x52, 0xab, 0x8f, 0x53, 0x0c, 0x35,
      0x9f, 0x08, 0x61, 0xd8, 0x07, 0xca, 0x0d, 0xbf, 0x50, 0x0d, 0x6a, 0x61,
      0x56, 0xa3, 0x8e, 0x08, 0x8a, 0x22, 0xb6, 0x5e, 0x52, 0xbc, 0x51, 0x4d,
      0x16, 0xcc, 0xf8, 0x06, 0x81, 0x8c, 0xe9, 0x1a, 0xb7, 0x79, 0x37, 0x36,
      0x5a, 0xf9, 0x0b, 0xbf, 0x74, 0xa3, 0x5b, 0xe6, 0xb4, 0x0b, 0x8e, 0xed,
      0xf2, 0x78, 0x5e, 0x42, 0x87, 0x4d,
  };
  for (usize i = 0; i < 114; i++)
    assert(text[i] == expected[i], "{u8:#04x} == {u8:#04x} (at index {usize})",
           text[i], expected[i], i);
}
