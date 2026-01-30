/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__SPINLOCK_H
#define UKO_OS_KERNEL__SPINLOCK_H 1

#include <types.h>

struct spinlock {
  atomic bool locked;
  u64 flags;

  // these only have meaning while `locked` is true
  u64 owner_hart_id;
  const char *file;
  usize line;
  const char *func;
};

struct spinlock_guard {
  struct spinlock *spinlock;
};

void __spinlock_guard_init(struct spinlock_guard *, struct spinlock *,
                           const char *, usize, const char *);

void __spinlock_guard_free(struct spinlock_guard *);

#define SPINLOCK_GUARDED(SPINLOCK_NAME, VARS)                                  \
  struct VARS __guarded_by_##SPINLOCK_NAME;                                    \
  struct spinlock SPINLOCK_NAME

#define SPINLOCK_GUARD(GUARD_NAME, OBJECT, SPINLOCK_NAME)                      \
  typeof((OBJECT)->__guarded_by_##SPINLOCK_NAME) *GUARD_NAME;                  \
  [[gnu::cleanup(__spinlock_guard_free)]]                                      \
  struct spinlock_guard __guard_##GUARD_NAME = {.spinlock = nullptr};          \
  __spinlock_guard_init(&__guard_##GUARD_NAME, &((OBJECT)->SPINLOCK_NAME),     \
                        __FILE__, __LINE__, __func__);                         \
  GUARD_NAME = &((OBJECT)->__guarded_by_##SPINLOCK_NAME)
#endif // UKO_OS_KERNEL__SPINLOCK_H
