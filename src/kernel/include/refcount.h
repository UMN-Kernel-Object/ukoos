/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__REFCOUNT_H
#define UKO_OS_KERNEL__REFCOUNT_H 1

#include <container.h>
#include <panic.h>
#include <print.h>
#include <stdatomic.h>

/**
 * A reference count. This essentially just an `atomic usize` that saturates at
 * `USIZE_MAX` (and stays there) instead of overflowing.
 */
typedef struct {
  atomic usize _count;
} refcount_t;

static inline void refcount_init(refcount_t *refcount) { refcount->_count = 1; }

static inline void refcount_incref(refcount_t *refcount) {
  for (;;) {
    usize count = atomic_load(&refcount->_count);
    assert(count);

    // If the counter saturated, don't overflow.
    if (unlikely(count == USIZE_MAX))
      return;

    if (likely(
            atomic_compare_exchange_weak(&refcount->_count, &count, count + 1)))
      if (unlikely(count + 1 == USIZE_MAX))
        print("refcount at {uptr} overflowed!", refcount);
    return;
  }
}

static inline void refcount_decref(refcount_t *refcount,
                                   void *(*free)(void *ctx), void *ctx) {
  usize count;
  do {
    count = atomic_load(&refcount->_count);
    assert(count);

    // If the counter saturated, don't decref.
    if (unlikely(count == USIZE_MAX))
      return;
  } while (unlikely(
      !atomic_compare_exchange_weak(&refcount->_count, &count, count - 1)));

  // The refcount is now zero.
  if (count == 1)
    free(ctx);
}

#endif // UKO_OS_KERNEL__REFCOUNT_H
