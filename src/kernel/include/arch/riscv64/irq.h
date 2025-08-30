#ifndef UKO_OS_KERNEL__IRQ_H
#define UKO_OS_KERNEL__IRQ_H 1

#include <insns.h>
#include <types.h>

constexpr u64 RISCV64_SSTATUS_SIE = (1UL << 1);

/**
 * irq_enable - Enables interrupt requests.
 *
 * Sets the SIE (supervisor interrupt enable) flag in the SSTATUS CSR.
 */
static inline void irq_enable(void) {
	csrs(RISCV64_CSR_SSTATUS, RISCV64_SSTATUS_SIE)
}

/**
 * irq_disable - Disables interrupt requests.
 *
 * Clears the SIE (supervisor interrupt enable) flag in the SSTATUS CSR.
 */
static inline void irq_disable(void) {
	csrc(RISCV64_CSR_SSTATUS, RISCV64_SSTATUS_SIE)
}

/**
 * get_irq_state - Gets the state of the SSTATUS SIE bit.
 *
 * Return: u32 1 if SIE bit in SSTATUS is set, 0 otherwise.
 */
static inline u32 get_irq_state(void) {
	u64 sstatus = csrr(RISCV64_CSR_SSTATUS);
	return (sstatus & RISCV64_SSTATUS_SIE) != 0;
}

#endif  // UKO_OS_KERNEL__IRQ_H

