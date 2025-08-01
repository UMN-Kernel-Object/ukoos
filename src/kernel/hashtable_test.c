/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hashtable.h>
#include <panic.h>
#include <print.h>
#include <selftest.h>

struct foo {
  i32 key;
  const char *value;
};

DEFINE_SELFTEST() {
  hashtable(foo) ht;
  struct foo *found_entry;
  assert(hashtable_init(&ht));

  assert(hashtable_find(foo, &ht, (struct foo){.key = 123}) == nullptr);

  assert(
      hashtable_insert(foo, &ht, (struct foo){.key = 123, .value = "Hello"}));
  found_entry = hashtable_find(foo, &ht, (struct foo){.key = 123});
  assert(found_entry);
  assert(strlen(found_entry->value) == 5);
  assert(memcmp(found_entry->value, "Hello", 5) == 0);

  assert(
      hashtable_insert(foo, &ht, (struct foo){.key = 456, .value = "World"}));
  found_entry = hashtable_find(foo, &ht, (struct foo){.key = 123});
  assert(found_entry);
  assert(strlen(found_entry->value) == 5);
  assert(memcmp(found_entry->value, "Hello", 5) == 0);
  found_entry = hashtable_find(foo, &ht, (struct foo){.key = 456});
  assert(found_entry);
  assert(strlen(found_entry->value) == 5);
  assert(memcmp(found_entry->value, "World", 5) == 0);

  assert(hashtable_find(foo, &ht, (struct foo){.key = 789}) == nullptr);
}

DEFINE_HASHTABLE_EQ(foo, lhs, rhs) { return lhs->key == rhs->key; }

DEFINE_HASHTABLE_HASH(foo, hasher, elem) { fnv1a_hash(hasher, elem->key); }

struct sieve_entry {
  usize n;
};

DEFINE_HASHTABLE_EQ(sieve_entry, lhs, rhs) { return lhs->n == rhs->n; }

DEFINE_HASHTABLE_HASH(sieve_entry, hasher, elem) {
  fnv1a_hash(hasher, elem->n);
}

DEFINE_SELFTEST() {
  hashtable(sieve_entry) sieve;

  assert(hashtable_init(&sieve));
  for (usize i = 2; i < 10'000; i++)
    assert(hashtable_insert(sieve_entry, &sieve, (struct sieve_entry){.n = i}));

  for (usize i = 2; i < 10'000; i++) {
    for (usize j = i; j < 10'000; j += i)
      assert(
          hashtable_delete(sieve_entry, &sieve, (struct sieve_entry){.n = j}));
  }

  assert(hashtable_len(&sieve) == 1229, "found {usize} primes under 10000",
         hashtable_len(&sieve));
}
