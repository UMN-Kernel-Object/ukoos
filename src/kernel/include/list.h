/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__LIST_H
#define UKO_OS_KERNEL__LIST_H 1

#include <container.h>
#include <panic.h>

/**
 * An intrusive doubly linked list. This is intended to be added to a struct as
 * a member to allow making lists of the struct, with one `list_head` that is
 * somewhere else to act as the owner of the list.
 *
 * Typically, such a list is the primary way ownership of the structs in the
 * list is managed.
 *
 * ### Example
 *
 * ```c
 * #include <list.h>
 *
 * struct foo {
 *   struct list_head list;
 *   int data;
 * };
 *
 * static struct list_head foos = LIST_INIT(foos);
 * ```
 */
struct list_head {
  /**
   * The previous node.
   */
  struct list_head *prev;

  /**
   * The next node.
   */
  struct list_head *next;
};

/**
 * An initializer expression for a `struct list_head`, that makes it
 * self-linked.
 *
 * Typically used for the `struct list_head` that is the list's owner.
 *
 * ### Example
 *
 * ```c
 * struct list_head objs = LIST_INIT(objs);
 *
 * struct parent {
 *   struct list_head children;
 * };
 *
 * struct parent global_parent = {
 *   .children = LIST_INIT(global_parent.children),
 * };
 *
 * void init_parent(struct parent *p) { p->children = LIST_INIT(p->children); }
 * ```
 */
#define LIST_INIT(LIST)                                                        \
  (struct list_head) { .prev = &(LIST), .next = &(LIST) }

/**
 * Returns whether `list` is empty.
 */
static inline bool list_is_empty(const struct list_head *list) {
  if (list == list->prev) {
    assert(list == list->next);
    return true;
  }
  if (list == list->next) {
    assert(list == list->prev);
    return true;
  }
  return false;
}

/**
 * Adds `elem` to the start of `list`.
 */
static inline void list_unshift(struct list_head *list,
                                struct list_head *elem) {
  assert(elem == elem->prev && elem == elem->next);
  elem->prev = list;
  elem->next = list->next;
  elem->prev->next = elem;
  elem->next->prev = elem;
}

/**
 * Adds `elem` to the end of `list`.
 */
static inline void list_push(struct list_head *list, struct list_head *elem) {
  assert(elem == elem->prev && elem == elem->next);
  elem->prev = list->prev;
  elem->next = list;
  elem->prev->next = elem;
  elem->next->prev = elem;
}

/**
 * Removes an element from the start of `list`. Panics if `list` is empty.
 */
static inline struct list_head *list_shift(struct list_head *list) {
  assert(list != list->prev && list != list->next);
  struct list_head *out = list->next;
  list->next = out->next;
  list->next->prev = list;
  *out = LIST_INIT(*out);
  return out;
}

/**
 * Removes an element from the end of `list`. Panics if `list` is empty.
 */
static inline struct list_head *list_pop(struct list_head *list) {
  assert(list != list->prev && list != list->next);
  struct list_head *out = list->prev;
  list->prev = out->prev;
  list->prev->next = list;
  *out = LIST_INIT(*out);
  return out;
}

/**
 * Removes `elem` from the list that contains it. Panics if `list` is the only
 * element in the list.
 */
static inline void list_remove(struct list_head *elem) {
  assert(elem != elem->prev && elem != elem->next);
  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
  *elem = LIST_INIT(*elem);
}

#endif // UKO_OS_KERNEL__LIST_H
