/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <panic.h>
#include <print.h>
#include <selftest.h>

extern const struct selftest _kernel_start_selftests[], _kernel_end_selftests[];

void run_selftests(void) {
  isize selftests_len = &_kernel_end_selftests[0] - &_kernel_start_selftests[0];
  assert(selftests_len >= 0);
  print("Found {isize} selftests.", selftests_len);
  for (usize i = 0; i < selftests_len; i++) {
    struct selftest selftest = _kernel_start_selftests[i];
    print("  {cstr}:{usize}", selftest.file, selftest.line);
    selftest.func();
  }
  print("All {isize} selftests passed.", selftests_len);
}

DEFINE_SELFTEST() {
  // An example self-test, to prove that self-tests themselves work!
  assert(1 + 1 == 2);
}
