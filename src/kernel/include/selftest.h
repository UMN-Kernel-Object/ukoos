/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__SELFTEST_H
#define UKO_OS_KERNEL__SELFTEST_H 1

#include <macro_utils.h>
#include <types.h>

#define _DEFINE_SELFTEST(N)                                                    \
  static void __PASTE(__selftest_func_, N)(void);                              \
  [[gnu::section(".selftests"), gnu::used]] static struct selftest __PASTE(    \
      __selftest_, N)[1] = {{                                                  \
      __PASTE(__selftest_func_, N),                                            \
      __FILE__,                                                                \
      __LINE__,                                                                \
  }};                                                                          \
  static void __PASTE(__selftest_func_, N)(void)
#define DEFINE_SELFTEST() _DEFINE_SELFTEST(__COUNTER__)

struct selftest {
  void (*func)(void);
  const char *file;
  usize line;
};

void run_selftests(void);

#endif // UKO_OS_KERNEL__SELFTEST_H
