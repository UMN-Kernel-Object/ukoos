/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
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
  RISCV64_CSR_FFLAGS = 0x001,
  RISCV64_CSR_FRM = 0x002,
  RISCV64_CSR_FCSR = 0x003,
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
  RISCV64_CSR_CYCLE = 0xc00,
  RISCV64_CSR_TIME = 0xc01,
  RISCV64_CSR_INSTRET = 0xc02,
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
 * Sets bits in a CSR, returning its previous value.
 *
 * @param csr The CSR to set.
 * @param mask A bitmask indicating which bits to set. (Any bit that is high
 *             in `mask` will be set in the CSR.)
 * @return The previous value of the CSR.
 */
static inline u64 csrrs(enum csr csr, u64 mask) {
  u64 out;
  // If this gets an "impossible constraint" error, make sure optimizations are
  // on -- this relies on being able to inline and constant-fold `csr`.
  __asm__ volatile("csrrs %0, %1, %2"
                   : "=r"(out)
                   : "i"(csr), "r"(mask)
                   : "memory");
  return out;
}

/**
 * Sets bits in a CSR.
 *
 * @param csr The CSR to set.
 * @param mask A bitmask indicating which bits to set. (Any bit that is high
 *             in `mask` will be set in the CSR.)
 */
static inline void csrs(enum csr csr, u64 mask) {
  // If this gets an "impossible constraint" error, make sure optimizations are
  // on -- this relies on being able to inline and constant-fold `csr`.
  __asm__ volatile("csrs %0, %1" ::"i"(csr), "r"(mask) : "memory");
}

/**
 * Clears a CSR, returning its previous value.
 *
 * @param csr The CSR to clear.
 * @param mask A bitmask indicating which bits to clear. (Any bit that is high
 *             in `mask` will be cleared in the CSR.)
 * @return The previous value of the CSR.
 */
static inline u64 csrrc(enum csr csr, u64 mask) {
  u64 out;
  // If this gets an "impossible constraint" error, make sure optimizations are
  // on -- this relies on being able to inline and constant-fold `csr`.
  __asm__ volatile("csrrc %0, %1, %2"
                   : "=r"(out)
                   : "i"(csr), "r"(mask)
                   : "memory");
  return out;
}

/**
 * Clears a CSR.
 *
 * @param csr The CSR to clear.
 * @param mask A bitmask indicating which bits to clear. (Any bit that is high
 *             in `mask` will be cleared in the CSR.)
 */
static inline void csrc(enum csr csr, u64 mask) {
  // If this gets an "impossible constraint" error, make sure optimizations are
  // on -- this relies on being able to inline and constant-fold `csr`.
  __asm__ volatile("csrc %0, %1" ::"i"(csr), "r"(mask) : "memory");
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

/**
 * Hints to the processor that we are in a spin-wait loop.
 */
static inline void pause_hint() {
  // ... with the caveat that our boards don't actually support `pause`,
  // which is in Zihintpause:
  // https://docs.riscv.org/reference/isa/unpriv/zihintpause.html
  // However, it behaves as a NOP on unsupported hardware, as far as I
  // can tell. GCC used to just let us call `__builtin_riscv_pause`,
  // but they stopped supporting this function on systems without
  // Zihintpause in 2023:
  // https://patchwork.sourceware.org/project/gcc/patch/22150b9f9e14718d368a0223848a69920d54305d.1691634361.git.research_trasio@irq.a4lg.com/
  // As a result, we use the raw instruction encoding for now.
  __asm__(".insn i 0x0f, 0, x0, x0, 0x010");
}
#endif // UKO_OS_KERNEL__ARCH_RISCV64_INSNS_H
