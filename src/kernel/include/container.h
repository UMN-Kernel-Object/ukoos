/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__CONTAINER_H
#define UKO_OS_KERNEL__CONTAINER_H 1

#include <compiler.h>

/**
 * Converts a pointer to a member to a pointer to the structure. Commonly used
 * with intrusive collections.
 *
 * - `ptr` is the pointer to convert.
 * - `type` is the type of the container.
 * - `member` is the member of the container that `ptr` points to.
 */
#define container_of(ptr, type, member)                                        \
  _Generic((ptr),                                                              \
      typeof(&((type *)nullptr)->member): (type *)((void *)(ptr) -             \
                                                   offsetof(type, member)),    \
      typeof(&((const type *)nullptr)->member): (                              \
               const type *)((const void *)(ptr) - offsetof(type, member)))

#endif // UKO_OS_KERNEL__CONTAINER_H
