/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <arch/riscv64/insns.h>
#include <arch/riscv64/trap_handler.h>
#include <panic.h>

enum trap_cause : u64 {
  // interrupts
  SUPERVISOR_SOFTWARE_INTERRUPT = (u64)1 << 63 | 1,
  SUPERVISOR_TIMER_INTERRUPT = (u64)1 << 63 | 5,
  SUPERVISOR_EXTERNAL_INTERRUPT = (u64)1 << 63 | 9,
  COUNTERFLOW_INTERRUPT = (u64)1 << 63 | 13,

  // exceptions
  // TODO: all the exceptions
  // the following is a placeholder
  EXCEPTION = (u64)0 << 63,
};

void trap_handler() {
  u64 scause = csrr(RISCV64_CSR_SCAUSE);
  switch (scause) {
  case SUPERVISOR_SOFTWARE_INTERRUPT:
    TODO();
  case SUPERVISOR_TIMER_INTERRUPT:
    TODO();
  case SUPERVISOR_EXTERNAL_INTERRUPT:
    external_trap();
  case COUNTERFLOW_INTERRUPT:
    TODO();
  }
}
