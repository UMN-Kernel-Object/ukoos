/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <arch/riscv64/insns.h>
#include <hart_locals.h>

/**
 * Storage for the boothart's hart-locals.
 */
struct hart_locals boothart_hart_locals;

struct hart_locals *get_hart_locals(void) {
  return (struct hart_locals *)csrr(RISCV64_CSR_SSCRATCH);
}

void init_boothart_hart_locals(u64 hart_id) {
  csrw(RISCV64_CSR_SSCRATCH, (u64)&boothart_hart_locals);
  boothart_hart_locals = (struct hart_locals){
      .hart_id = hart_id,
      .heap = nullptr,
      .rng = {},
  };
}

void init_hart_locals(u64 hart_id);
