/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__HASHTABLE_H
#define UKO_OS_KERNEL__HASHTABLE_H 1

#include <crypto/subtle/fnv1a.h>

/**
 * An implementation of hashtables based on the ones present in Go 1.24. That
 * design is based on Google's Swiss Table design, with some modifications made
 * to limit the latency impact of a table resize.
 *
 * The exact implementation of this hashtable depends on the architecture; in
 * particular, on the architecture's SIMD/SWAR types.
 */

/**
 * A generic hash table. If this would be resized to contain more than 1024
 * buckets, the hashtable is instead split, which is handled by
 * `struct generic_hashtable_tree`.
 */
struct generic_hashtable {
  /**
   * The metadata array. For every entry in `slots`, there is one byte. The
   * MSB bit stores "is this a special value," the low 7 bits store 7 bits of
   * hash.
   */
  void *control_words;

  /**
   * A pointer to the slots array. The size of each element is determined at
   * the call-site, so we don't have a type here.
   */
  void *slots;

  /**
   * The log2 of the number of groups in the `control_bytes` and `slots` arrays.
   */
  u8 log2_group_count;

  /**
   * The number of elements in the hashtable.
   */
  u8 len;
};

/**
 * A tree of generic hashtables.
 */
struct generic_hashtable_tree {
  /**
   * The leaves of the tree.
   */
  struct generic_hashtable *hashtables;

  /**
   * The number of elements in all hashtables.
   */
  usize len;

  /**
   * A mutation count, used to catch iterator invalidation issues.
   *
   * This only needs to count inserts and deletes, not merely modifying entries.
   */
  u32 mutation_count;

  /**
   * The maximum depth of the tree.
   */
  u8 depth;

  /**
   * The initial state of the hasher after hashing the random seed.
   */
  struct fnv1a_hasher hasher;
};

[[gnu::access(read_write, 1), gnu::access(read_only, 4), gnu::nonnull(1, 3, 4)]]
bool _hashtable_delete(struct generic_hashtable_tree *tree, usize entry_size,
                       [[gnu::access(read_only, 1), gnu::access(read_only, 2),
                         gnu::nonnull(1, 2)]] bool (*eq_entry)(const void *lhs,
                                                               const void *rhs),
                       const void *key_entry, u64 key_entry_hash,
                       void *out_old_entry);

[[gnu::access(read_only, 1), gnu::access(read_only, 4), gnu::nonnull(1, 3, 4)]]
void *_hashtable_find(struct generic_hashtable_tree *tree, usize entry_size,
                      [[gnu::access(read_only, 1), gnu::access(read_only, 2),
                        gnu::nonnull(1, 2)]] bool (*eq_entry)(const void *lhs,
                                                              const void *rhs),
                      const void *key_entry, u64 key_entry_hash);

[[gnu::access(write_only, 1), gnu::nonnull(1),
  nodiscard("The return value of hashtable_init specifies whether the "
            "hashtable was successfully inititalized")]]
bool _hashtable_init(struct generic_hashtable_tree *tree, usize entry_size);

[[gnu::access(read_write, 1), gnu::access(read_only, 5),
  gnu::access(write_only, 7), gnu::access(write_only, 8),
  gnu::nonnull(1, 3, 4, 5),
  nodiscard("The return value of hashtable_insert specifies whether the insert "
            "successfully completed")]]
bool _hashtable_insert(
    struct generic_hashtable_tree *tree, usize entry_size,
    [[gnu::access(read_only, 1), gnu::access(read_only, 2),
      gnu::nonnull(1, 2)]] bool (*eq_entry)(const void *lhs, const void *rhs),
    [[gnu::access(read_write, 1), gnu::access(read_only, 2),
      gnu::nonnull(1)]] void (*hash_entry)(struct fnv1a_hasher *hasher,
                                           const void *entry),
    const void *new_entry, u64 new_entry_hash, bool *out_had_entry,
    void *out_old_entry);

static inline usize _hashtable_len(const struct generic_hashtable_tree *tree) {
  return tree->len;
}

#define hashtable(ENTRY_STRUCT_NAME)                                           \
  struct hashtableᐸ##ENTRY_STRUCT_NAME##ᐳ {                                    \
    union _hashtableᐸ##ENTRY_STRUCT_NAME##ᐳ {                                  \
      struct generic_hashtable_tree generic;                                   \
      struct ENTRY_STRUCT_NAME *type;                                          \
    } _hashtable;                                                              \
  }

/**
 * Tries to find `KEY` in `HASHTABLE`, deleting the matching entry if it was
 * found. Returns whether an entry was deleted. If an entry was deleted and
 * `OLD_ENTRY` was not `nullptr`, writes it to `OLD_ENTRY`.
 */
#define hashtable_delete_and_return(ENTRY_STRUCT_NAME, HASHTABLE, KEY,         \
                                    OLD_ENTRY)                                 \
  ({                                                                           \
    /* Import the eq and hash functions. */                                    \
    [[gnu::access(read_only, 1), gnu::access(read_only, 2),                    \
      gnu::nonnull(1, 2),                                                      \
      gnu::pure]] bool __hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ(const void *lhs,  \
                                                             const void *rhs); \
    [[gnu::access(read_write, 1), gnu::access(read_only, 2),                   \
      gnu::nonnull(1, 2)]] void                                                \
        __hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(struct fnv1a_hasher *hasher,   \
                                                const void *entry);            \
                                                                               \
    /* Evaluate the arguments. */                                              \
    hashtable(ENTRY_STRUCT_NAME) *__hashtable_delete_and_return_HASHTABLE =    \
        (HASHTABLE);                                                           \
    struct ENTRY_STRUCT_NAME __hashtable_delete_and_return_KEY = (KEY);        \
    struct ENTRY_STRUCT_NAME *__hashtable_delete_and_return_OLD_ENTRY =        \
        (OLD_ENTRY);                                                           \
                                                                               \
    /* Compute the hash of the key. */                                         \
    struct fnv1a_hasher __hashtable_delete_and_return_HASHER =                 \
        __hashtable_delete_and_return_HASHTABLE->_hashtable.generic.hasher;    \
    __hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(                                   \
        &__hashtable_delete_and_return_HASHER,                                 \
        &__hashtable_delete_and_return_KEY);                                   \
                                                                               \
    _hashtable_delete(                                                         \
        &__hashtable_delete_and_return_HASHTABLE->_hashtable.generic,          \
        sizeof(struct ENTRY_STRUCT_NAME),                                      \
        __hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ,                                 \
        &__hashtable_delete_and_return_KEY,                                    \
        __hashtable_delete_and_return_HASHER.hash,                             \
        __hashtable_delete_and_return_OLD_ENTRY);                              \
  })

/**
 * Tries to find `KEY` in `HASHTABLE`, deleting the matching entry if it was
 * found. Returns whether an entry was deleted.
 */
#define hashtable_delete(ENTRY_STRUCT_NAME, HASHTABLE, KEY, ...)               \
  hashtable_delete_and_return(ENTRY_STRUCT_NAME, HASHTABLE,                    \
                              (KEY __VA_OPT__(, ) __VA_ARGS__), nullptr)

/**
 * Tries to find `KEY` in `HASHTABLE`, returning a pointer to the matching entry
 * if it was found, and `nullptr` if not.
 */
#define hashtable_find(ENTRY_STRUCT_NAME, HASHTABLE, KEY, ...)                 \
  ({                                                                           \
    /* Import the eq and hash functions. */                                    \
    [[gnu::access(read_only, 1), gnu::access(read_only, 2),                    \
      gnu::nonnull(1, 2),                                                      \
      gnu::pure]] bool __hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ(const void *lhs,  \
                                                             const void *rhs); \
    [[gnu::access(read_write, 1), gnu::access(read_only, 2),                   \
      gnu::nonnull(1, 2)]] void                                                \
        __hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(struct fnv1a_hasher *hasher,   \
                                                const void *entry);            \
                                                                               \
    /* Evaluate the arguments. */                                              \
    hashtable(ENTRY_STRUCT_NAME) *__hashtable_find_HASHTABLE = (HASHTABLE);    \
    struct ENTRY_STRUCT_NAME __hashtable_find_KEY =                            \
        (KEY __VA_OPT__(, ) __VA_ARGS__);                                      \
                                                                               \
    /* Compute the hash of the key. */                                         \
    struct fnv1a_hasher __hashtable_find_HASHER =                              \
        __hashtable_find_HASHTABLE->_hashtable.generic.hasher;                 \
    __hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(&__hashtable_find_HASHER,          \
                                            &__hashtable_find_KEY);            \
                                                                               \
    struct ENTRY_STRUCT_NAME *__hashtable_find_ENTRY =                         \
        _hashtable_find(&__hashtable_find_HASHTABLE->_hashtable.generic,       \
                        sizeof(struct ENTRY_STRUCT_NAME),                      \
                        __hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ,                 \
                        &__hashtable_find_KEY, __hashtable_find_HASHER.hash);  \
    __hashtable_find_ENTRY;                                                    \
  })

/**
 * Tries to initialize `HASHTABLE`, returning whether it was successful.
 *
 * TODO: Once [0] is fixed, define this as a stmtexpr so that it doesn't expand
 * HASHTABLE twice.
 *
 * [0]: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94379
 */
#define hashtable_init(HASHTABLE)                                              \
  _hashtable_init(&(HASHTABLE)->_hashtable.generic,                            \
                  sizeof(*(HASHTABLE)->_hashtable.type))

/**
 * Tries to insert `ENTRY` into `HASHTABLE`, returning whether it was
 * successful. Writes whether an entry existed into `HAD_ENTRY`. If it did and
 * `OLD_ENTRY` was not `nullptr`, writes it into `OLD_ENTRY`.
 */
#define hashtable_insert_and_return(ENTRY_STRUCT_NAME, HASHTABLE, ENTRY,       \
                                    HAD_ENTRY, OLD_ENTRY)                      \
  ({                                                                           \
    /* Import the eq and hash functions. */                                    \
    [[gnu::access(read_only, 1), gnu::access(read_only, 2),                    \
      gnu::nonnull(1, 2),                                                      \
      gnu::pure]] bool __hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ(const void *lhs,  \
                                                             const void *rhs); \
    [[gnu::access(read_write, 1), gnu::access(read_only, 2),                   \
      gnu::nonnull(1, 2)]] void                                                \
        __hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(struct fnv1a_hasher *hasher,   \
                                                const void *entry);            \
                                                                               \
    /* Evaluate the arguments. */                                              \
    hashtable(ENTRY_STRUCT_NAME) *__hashtable_insert_and_return_HASHTABLE =    \
        (HASHTABLE);                                                           \
    struct ENTRY_STRUCT_NAME __hashtable_insert_and_return_ENTRY = (ENTRY);    \
    bool *__hashtable_insert_and_return_HAD_ENTRY = (HAD_ENTRY);               \
    struct ENTRY_STRUCT_NAME *__hashtable_insert_and_return_OLD_ENTRY =        \
        (OLD_ENTRY);                                                           \
                                                                               \
    /* Compute the hash of the key. */                                         \
    struct fnv1a_hasher __hashtable_insert_and_return_HASHER =                 \
        __hashtable_insert_and_return_HASHTABLE->_hashtable.generic.hasher;    \
    __hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(                                   \
        &__hashtable_insert_and_return_HASHER,                                 \
        &__hashtable_insert_and_return_ENTRY);                                 \
                                                                               \
    _hashtable_insert(                                                         \
        &__hashtable_insert_and_return_HASHTABLE->_hashtable.generic,          \
        sizeof(struct ENTRY_STRUCT_NAME),                                      \
        __hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ,                                 \
        __hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ,                               \
        &__hashtable_insert_and_return_ENTRY,                                  \
        __hashtable_insert_and_return_HASHER.hash,                             \
        __hashtable_insert_and_return_HAD_ENTRY,                               \
        __hashtable_insert_and_return_OLD_ENTRY);                              \
  })

/**
 * Tries to insert `ENTRY` into `HASHTABLE`, returning whether it was
 * successful.
 */
#define hashtable_insert(ENTRY_STRUCT_NAME, HASHTABLE, ENTRY, ...)             \
  hashtable_insert_and_return(ENTRY_STRUCT_NAME, HASHTABLE,                    \
                              (ENTRY __VA_OPT__(, ) __VA_ARGS__), nullptr,     \
                              nullptr)

/**
 * Returns the number of elements in `HASHTABLE`.
 */
#define hashtable_len(HASHTABLE)                                               \
  ({                                                                           \
    auto __hashtable_len_HASHTABLE = (HASHTABLE);                              \
    __hashtable_len_HASHTABLE->_hashtable.generic.len;                         \
  })

/**
 * A helper for defining equality checks for a type used with hashtables. This
 * does not need to be visible to the uses of the hashtables of that type, since
 * it won't be inlined without LTO regardless.
 *
 * ### Example
 *
 * ```
 * struct foo {
 *   i32 key;
 *   i32 val;
 * };
 *
 * DEFINE_HASHTABLE_EQ(foo, lhs, rhs) { return lhs->key == rhs->key; }
 * ```
 */
#define DEFINE_HASHTABLE_EQ(ENTRY_STRUCT_NAME, LHS, RHS)                       \
  [[gnu::access(read_only, 1), gnu::access(read_only, 2), gnu::nonnull(1, 2),  \
    gnu::pure]] static bool                                                    \
      _hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ(                                    \
          const struct ENTRY_STRUCT_NAME *lhs,                                 \
          const struct ENTRY_STRUCT_NAME *rhs);                                \
  [[gnu::access(read_only, 1), gnu::access(read_only, 2), gnu::nonnull(1, 2),  \
    gnu::pure]] bool __hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ(const void *lhs,    \
                                                           const void *rhs) {  \
    return _hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ(lhs, rhs);                     \
  }                                                                            \
  [[gnu::access(read_only, 1), gnu::access(read_only, 2), gnu::nonnull(1, 2),  \
    gnu::pure]] static bool                                                    \
      _hashtable_eqᐸ##ENTRY_STRUCT_NAME##ᐳ(                                    \
          const struct ENTRY_STRUCT_NAME *LHS,                                 \
          const struct ENTRY_STRUCT_NAME *RHS)

/**
 * A helper for defining hash functions for a type used with hashtables. This
 * does not need to be visible to the uses of the hashtables of that type, since
 * it won't be inlined without LTO regardless.
 *
 * ### Example
 *
 * ```
 * struct foo {
 *   i32 key;
 *   i32 val;
 * };
 *
 * DEFINE_HASHTABLE_HASH(foo, hasher, entry) {
 *   fnv1a_hash(hasher, entry->key);
 * }
 * ```
 */
#define DEFINE_HASHTABLE_HASH(ENTRY_STRUCT_NAME, HASHER, ENTRY)                \
  [[gnu::access(read_write, 1), gnu::access(read_only, 2),                     \
    gnu::nonnull(1, 2)]] static void                                           \
      _hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(                                  \
          struct fnv1a_hasher *hasher, const struct ENTRY_STRUCT_NAME *entry); \
  [[gnu::access(read_write, 1), gnu::access(read_only, 2),                     \
    gnu::nonnull(1, 2)]] void                                                  \
      __hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(struct fnv1a_hasher *hasher,     \
                                              const void *entry) {             \
    return _hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(hasher, entry);              \
  }                                                                            \
  [[gnu::access(read_write, 1), gnu::access(read_only, 2),                     \
    gnu::nonnull(1, 2)]] static void                                           \
      _hashtable_hashᐸ##ENTRY_STRUCT_NAME##ᐳ(                                  \
          struct fnv1a_hasher *HASHER, const struct ENTRY_STRUCT_NAME *ENTRY)

#endif // UKO_OS_KERNEL__HASHTABLE_H
