/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__INIT_H
#define UKO_OS_KERNEL__INIT_H 1

#include <macro_utils.h>
#include <types.h>

/**
 * The pre-defined priority levels.
 */
enum initializer_priority : i32 {
  /**
   * The priority level at which drivers should register themselves.
   */
  INIT_REGISTER_DRIVERS = 10,
  INIT_REGISTER_PCI_DRIVERS = 20
};

/**
 * An initializer function.
 */
struct initializer {
  /**
   * The actual function to run.
   */
  void (*func)(void);

  /**
   * The path to the file the initializer is defined in.
   */
  const char *file;

  /**
   * The line the initializer is defined on.
   */
  u32 line;

  /**
   * The priority of the initializer.
   */
  enum initializer_priority priority;
};

#define _DEFINE_INIT(PRIORITY, N)                                              \
  static_assert(__builtin_classify_type(typeof((PRIORITY))) ==                 \
                    __builtin_classify_type(enum initializer_priority),        \
                "PRIORITY must be an enum initializer_priority");              \
  static_assert(                                                               \
      _Generic((PRIORITY), enum initializer_priority: true, default: false),   \
      "PRIORITY must be an enum initializer_priority");                        \
                                                                               \
  static void __PASTE(__init_func_, N)(void);                                  \
  [[gnu::section(".initializers,\"wa\",@progbits#"), gnu::used]]               \
  static struct initializer __PASTE(__initializer_, N)[1] = {{                 \
      __PASTE(__init_func_, N),                                                \
      __FILE__,                                                                \
      __LINE__,                                                                \
      (PRIORITY),                                                              \
  }};                                                                          \
  static void __PASTE(__init_func_, N)(void)

/**
 * Defines an initializer. `PRIORITY` should be an `enum initializer_priority`.
 */
#define DEFINE_INIT(PRIORITY) _DEFINE_INIT(PRIORITY, __COUNTER__)

/**
 * Runs all initializers.
 */
void run_initializers(void);

#endif // UKO_OS_KERNEL__INIT_H
