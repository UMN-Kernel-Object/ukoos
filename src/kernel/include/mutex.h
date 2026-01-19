/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MUTEX_H
#define UKO_OS_KERNEL__MUTEX_H 1

#include <types.h>

struct mutex {};

struct mutex_guard {
  struct mutex *mutex;
};

void __mutex_guard_init(struct mutex_guard *, struct mutex *);
void __mutex_guard_free(struct mutex_guard *);

#define MUTEX_GUARDED(MUTEX_NAME, VARS)                                        \
  struct VARS __guarded_by_##MUTEX_NAME;                                       \
  struct mutex MUTEX_NAME

#define MUTEX_GUARD(GUARD_NAME, OBJECT, MUTEX_NAME)                            \
  typeof((OBJECT)->__guarded_by_##MUTEX_NAME) *GUARD_NAME;                     \
  [[gnu::cleanup(__mutex_guard_free)]]                                         \
  struct mutex_guard __guard_##GUARD_NAME = {.mutex = nullptr};                \
  __mutex_guard_init(&__guard_##GUARD_NAME, &((OBJECT)->MUTEX_NAME));          \
  GUARD_NAME = &((OBJECT)->__guarded_by_##MUTEX_NAME)

#endif // UKO_OS_KERNEL__MUTEX_H
