/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <print.h>
#include <types.h>

uptr _backtrace_begin(void) {
  uptr *fp;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
  register uptr reg_fp __asm__("s0");
  fp = (uptr *)reg_fp;
#pragma GCC diagnostic pop

  return (uptr)fp[-2];
}

uptr _backtrace_next(uptr fp_value) {
  uptr *fp = (uptr *)fp_value;
  return fp[-2];
}

uaddr _backtrace_pc(uptr fp_value) {
  uptr *fp = (uptr *)fp_value;
  return (uaddr)fp[-1];
}
