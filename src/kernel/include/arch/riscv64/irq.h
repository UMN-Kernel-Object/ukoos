/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__ARCH_RISCV64_IRQ_H
#define UKO_OS_KERNEL__ARCH_RISCV64_IRQ_H 1

/**
 * IRQ handling.
 */

#include <arch/riscv64/insns.h>
#include <types.h>

// TODO: move this over to insns.h?
#define RISCV64_CSR_SSTATUS_SIE (1 << 1)

// TODO: consider making these common across architectures somehow?
// linux has `arch_local_irq­_*` and then calls those from `local_irq_*`
// for now, riscv64 is the only target anyways, so ...

/**
 * Disables local interrupts.
 */
static inline void local_irq_disable(void) {
  csrc(RISCV64_CSR_SSTATUS, RISCV64_CSR_SSTATUS_SIE); // clear SIE bit
}

/**
 * Enables local interrupts.
 */
static inline void local_irq_enable(void) {
  csrs(RISCV64_CSR_SSTATUS, RISCV64_CSR_SSTATUS_SIE); // set SIE bit
}

/**
 * Saves the current interrupt state and disables local interrupts.
 *
 * @return The previous interrupt state flags.
 */
static inline u64 local_irq_save(void) {
  u64 sstatus =
      csrrc(RISCV64_CSR_SSTATUS, RISCV64_CSR_SSTATUS_SIE); // clear SIE bit
  return sstatus & RISCV64_CSR_SSTATUS_SIE;
}

/**
 * Restores the local interrupt state from the given flags.
 */
static inline void local_irq_restore(u64 flags) {
  if (flags & RISCV64_CSR_SSTATUS_SIE) {
    local_irq_enable();
  }
}

#endif // UKO_OS_KERNEL__ARCH_RISCV64_IRQ_H
