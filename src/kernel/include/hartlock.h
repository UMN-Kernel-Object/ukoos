/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__HARTLOCK_H
#define UKO_OS_KERNEL__HARTLOCK_H 1

#include <types.h>

struct hartlock {
  u64 flags;
};

// None of these functions should be called manually.
// See doc/kernel/containers/guards.md for usage.

void __hartlock_init(struct hartlock *);

void __hartlock_free(struct hartlock *);

#define HARTLOCK_GUARDED(VARS)                                                 \
  struct VARS __guarded_by_hartlock;                                           \

#define HARTLOCK_GUARD(GUARD_NAME, OBJECT)                                     \
  typeof((OBJECT)->__guarded_by_hartlock) *GUARD_NAME;                         \
  [[gnu::cleanup(__hartlock_free)]]                                            \
  struct hartlock __my_hartlock = { 0 };                                       \
  __hartlock_init(&__my_hartlock);                                             \
  GUARD_NAME = &((OBJECT)->__guarded_by_hartlock)

#define WITH_HARTLOCK(LOCK_NAME)                                               \
  [[gnu::cleanup(__hartlock_free)]]                                            \
  struct hartlock __my_hartlock = { 0 };                                       \
  __hartlock_init(&__my_hartlock);                                             \
  struct hartlock *LOCK_NAME = &__my_hartlock

#endif // UKO_OS_KERNEL__HARTLOCK_H
