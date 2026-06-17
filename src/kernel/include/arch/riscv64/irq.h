#ifndef UKO_OS_KERNEL__IRQ_H
#define UKO_OS_KERNEL__IRQ_H 1

#include <insns.h>
#include <types.h>

constexpr u64 RISCV64_SSTATUS_SIE = (1UL << 1);

/**
 * enable_interrupts - Enables interrupt requests.
 *
 * Sets the SIE (supervisor interrupt enable) flag in the SSTATUS CSR.
 */
static inline void enable_interrupts(void) {
  csrs(RISCV64_CSR_SSTATUS, RISCV64_SSTATUS_SIE);
}

/**
 * disable_interrupts - Disables interrupt requests.
 *
 * Clears the SIE (supervisor interrupt enable) flag in the SSTATUS CSR.
 */
static inline void disable_interrupts(void) {
  csrc(RISCV64_CSR_SSTATUS, RISCV64_SSTATUS_SIE);
}

/**
 * are_interrupts_enabled - Checks if interrupt requests are enabled.
 *
 * Return: true if interrupt requests are enabled, false otherwise.
 */
static inline bool are_interrupts_enabled(void) {
  u64 sstatus = csrr(RISCV64_CSR_SSTATUS);
  return (sstatus & RISCV64_SSTATUS_SIE) != 0;
}

#endif  // UKO_OS_KERNEL__IRQ_H
