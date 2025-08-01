/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hashtable.h>
#include <mm/alloc.h>
#include <panic.h>
#include <random.h>
#include <swar.h>

/**
 * An implementation of hashtables that uses SWAR.
 */

constexpr usize ENTRIES_IN_GROUP = 8;
constexpr usize INIT_LOG2_GROUP_COUNT = 1;
constexpr usize INIT_ENTRIES = ENTRIES_IN_GROUP << INIT_LOG2_GROUP_COUNT;

/** The special values for a metadata byte. */
enum control : u8 {
  /**
   * The control byte used when an entry does not contain a value.
   */
  CTRL_EMPTY = 0b1'0000000,

  /**
   * A placeholder value used to get `u8x8_find_first_match` to skip an entry.
   * This should never be written to an actual table.
   */
  CTRL_SWAR_PLACEHOLDER = 0b1'0000001,

  /**
   * The control byte used when an entry does not contain a value, but
   * formerly did, and that value was deleted when the group was full.
   *
   * This indicates that a probe has to go to the next group.
   */
  CTRL_TOMBSTONE = 0b1'1111111,
};

/**
 * The metadata bytes for a single group.
 */
union control_word {
  u8x8 u8x8;
  enum control controls[8];
};

static_assert(sizeof(union control_word) == ENTRIES_IN_GROUP);

/**
 * Tries to initialize a single table. Returns whether allocation succeeded.
 */
[[nodiscard]]
static bool hashtable_init_table(struct generic_hashtable *table,
                                 usize entry_size) {
  void *allocation = alloc(INIT_ENTRIES + INIT_ENTRIES * entry_size);
  if (!allocation)
    return false;
  memset(allocation, CTRL_EMPTY, INIT_ENTRIES);
  *table = (struct generic_hashtable){
      .control_words = allocation,
      .slots = allocation + INIT_ENTRIES,
      .log2_group_count = INIT_LOG2_GROUP_COUNT,
      .len = 1,
  };
  return true;
}

/**
 * The pointers to the parts of a slot (metadata byte and the actual slot).
 */
struct slot_ptrs {
  enum control *control;
  void *slot;
};

static struct slot_ptrs hashtable_slot_ptrs(struct generic_hashtable *table,
                                            usize entry_size, usize group_index,
                                            usize match_index) {
  assert(group_index < (1 << table->log2_group_count), "group_index = {usize}",
         group_index);
  assert(0 < match_index && match_index <= ENTRIES_IN_GROUP,
         "match_index = {usize}", match_index);

  usize i = group_index * ENTRIES_IN_GROUP + (match_index - 1);
  return (struct slot_ptrs){
      .control = table->control_words + i,
      .slot = table->slots + (entry_size * i),
  };
}

bool _hashtable_delete(struct generic_hashtable_tree *tree, usize entry_size,
                       [[gnu::access(read_only, 1), gnu::access(read_only, 2),
                         gnu::nonnull(1, 2)]] bool (*eq_entry)(const void *lhs,
                                                               const void *rhs),
                       const void *key_entry, u64 key_entry_hash,
                       void *out_old_entry) {
  if (1)
    TODO();
  return false;
}

void *_hashtable_find(struct generic_hashtable_tree *tree, usize entry_size,
                      [[gnu::access(read_only, 1), gnu::access(read_only, 2),
                        gnu::nonnull(1, 2)]] bool (*eq_entry)(const void *lhs,
                                                              const void *rhs),
                      const void *key_entry, u64 key_entry_hash) {
  // Split the hash into low, middle, and high bits.
  //
  // - The low bits are used to accelerate lookups within a group.
  // - The middle bits are used to find a group within a hashtable.
  // - The high bits are used to find a hashtable within a hashtable tree.
  const u8 low_hash_bits = key_entry_hash & 0x7f;
  const usize mid_hash_bits = (key_entry_hash >> 7) & 0x7f;
  const usize high_hash_bits =
      (key_entry_hash >> 14) & (((usize)1 << tree->depth) - 1);

  // Find the hashtable.
  struct generic_hashtable *table = &tree->hashtables[high_hash_bits];

  // Find the group.
  const usize group_index_mask = ((usize)1 << table->log2_group_count) - 1;
  const usize initial_group_index = mid_hash_bits & group_index_mask;
  usize group_index = initial_group_index;
  for (;;) {
    union control_word *control_word =
        &((union control_word *)table->control_words)[group_index];

    usize match_index =
        u8x8_find_first_match(control_word->u8x8, low_hash_bits);
    if (match_index) {
      struct slot_ptrs ptrs =
          hashtable_slot_ptrs(table, entry_size, group_index, match_index);

      if (eq_entry(key_entry, ptrs.slot)) {
        return ptrs.slot;
      } else {
        TODO("{usize} {u8:#04x}", match_index, low_hash_bits);
      }
    }

    if (likely(u8x8_contains_match(control_word->u8x8, CTRL_EMPTY))) {
      // The group was never full, so we don't have to search the next group to
      // know that the entry does not exist.
      return nullptr;
    } else {
      TODO("{usize} {u8:#04x}", match_index, low_hash_bits);
    }
  }
}

bool _hashtable_init(struct generic_hashtable_tree *tree, usize entry_size) {
  struct generic_hashtable *hashtables =
      alloc(sizeof(struct generic_hashtable));
  if (!hashtables)
    return false;
  if (!hashtable_init_table(hashtables, entry_size)) {
    free(hashtables);
    return false;
  }

  *tree = (struct generic_hashtable_tree){
      .hashtables = hashtables,
      .len = 0,
      .mutation_count = 0,
      .depth = 0,
  };
  getrandom((u8 *)&tree->hasher, sizeof(tree->hasher));
  return true;
}

bool _hashtable_insert(
    struct generic_hashtable_tree *tree, usize entry_size,
    [[gnu::access(read_only, 1), gnu::access(read_only, 2),
      gnu::nonnull(1, 2)]] bool (*eq_entry)(const void *lhs, const void *rhs),
    [[gnu::access(read_write, 1), gnu::access(read_only, 2),
      gnu::nonnull(1)]] void (*hash_entry)(struct fnv1a_hasher *hasher,
                                           const void *entry),
    const void *new_entry, u64 new_entry_hash, bool *out_had_entry,
    void *out_old_entry) {
  // Split the hash into low, middle, and high bits.
  //
  // - The low bits are used to accelerate lookups within a group.
  // - The middle bits are used to find a group within a hashtable.
  // - The high bits are used to find a hashtable within a hashtable tree.
  const u8 low_hash_bits = new_entry_hash & 0x7f;
  const usize mid_hash_bits = (new_entry_hash >> 7) & 0x7f;
  const usize high_hash_bits =
      (new_entry_hash >> 14) & (((usize)1 << tree->depth) - 1);

  // Find the hashtable.
  struct generic_hashtable *table = &tree->hashtables[high_hash_bits];

  // Find the group.
  const usize group_index_mask = ((usize)1 << table->log2_group_count) - 1;
  const usize initial_group_index = mid_hash_bits & group_index_mask;
  usize group_index = initial_group_index;
  for (;;) {
    union control_word *control_word =
        &((union control_word *)table->control_words)[group_index];

    usize match_index =
        u8x8_find_first_match(control_word->u8x8, low_hash_bits);
    if (match_index) {
      TODO("{usize} {u8:#04x}", match_index, low_hash_bits);
    }

    usize empty_index = u8x8_find_first_match(control_word->u8x8, CTRL_EMPTY);
    if (likely(empty_index)) {
      // Check if we have to resize.
      if ((table->len + 1) ==
          ((ENTRIES_IN_GROUP - 1) << table->log2_group_count)) {
        usize new_entries = ENTRIES_IN_GROUP << (table->log2_group_count + 1);
        void *new_allocation = alloc(new_entries + new_entries * entry_size);
        if (!new_allocation)
          return false;
        memset(new_allocation, CTRL_EMPTY, new_entries);
        TODO();
        *table = (struct generic_hashtable){
            .control_words = new_allocation,
            .slots = new_allocation + new_entries,
            .log2_group_count = INIT_LOG2_GROUP_COUNT,
            .len = 1,
        };
        return true;
      }

      // If not, store the element in the slot.
      // void *slot = hashtable_slot(table, entry_size, group_index,
      // empty_index);
      table->len++;
      tree->len++;
      control_word->controls[empty_index - 1] = low_hash_bits;
      // memcpy(slot, new_entry, entry_size);
      if (out_had_entry)
        *out_had_entry = true;
      return true;
    } else {
      TODO("{usize} {u8:#04x}", match_index, low_hash_bits);
    }
  }
}
