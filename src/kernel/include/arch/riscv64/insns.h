/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__ARCH_RISCV64_INSNS_H
#define UKO_OS_KERNEL__ARCH_RISCV64_INSNS_H 1

/**
 * Bindings to RISC-V instructions.
 */

#include <types.h>

/**
 * The CSRs we use.
 */
enum csr {
  RISCV64_CSR_SSTATUS = 0x100,
  RISCV64_CSR_SIE = 0x104,
  RISCV64_CSR_STVEC = 0x105,
  RISCV64_CSR_SCOUNTEREN = 0x106,
  RISCV64_CSR_SENVCFG = 0x10a,
  RISCV64_CSR_SSCRATCH = 0x140,
  RISCV64_CSR_SEPC = 0x141,
  RISCV64_CSR_SCAUSE = 0x142,
  RISCV64_CSR_STVAL = 0x143,
  RISCV64_CSR_SIP = 0x144,
  RISCV64_CSR_SATP = 0x180,
};

/**
 * Reads a CSR.
 */
static inline u64 csrr(enum csr csr) {
  u64 out;
  // If this gets an "impossible constraint" error, make sure optimizations are
  // on -- this relies on being able to inline and constant-fold `csr`.
  __asm__ volatile("csrr %0, %1" : "=r"(out) : "i"(csr) : "memory");
  return out;
}

/**
 * Writes to a CSR.
 */
static inline void csrw(enum csr csr, u64 value) {
  // If this gets an "impossible constraint" error, make sure optimizations are
  // on -- this relies on being able to inline and constant-fold `csr`.
  __asm__ volatile("csrw %0, %1" ::"i"(csr), "r"(value) : "memory");
}

/**
 * Fences modifications to page tables.
 */
static inline void sfence_vma(void) {
  __asm__ volatile("sfence.vma" ::: "memory");
}

/**
 * Halts execution until an enabled interrupt is pending. Does not require
 * that interrupts are globally enabled. May spuriously wake up.
 */
static inline void wfi(void) { __asm__ volatile("wfi" :::); }

#endif // UKO_OS_KERNEL__ARCH_RISCV64_INSNS_H
