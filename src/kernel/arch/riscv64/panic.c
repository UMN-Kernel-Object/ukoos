/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <arch/riscv64/insns.h>
#include <arch/riscv64/sbi.h>

[[noreturn]]
void _panic_halt(void) {
  // Power off.
  sbi_call(0x53525354, 0, 0, 1);

  // Loop forever.
  for (;;) {
    wfi();
  }
}

void _panic_begin(void) {
  // If we get an exception, just go to the halt function anyway.
  csrw(RISCV64_CSR_STVEC, (uptr)_panic_halt);
}
