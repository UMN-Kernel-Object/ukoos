/*
 * SPDX-FileCopyrightText: ukoOS Contributors
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
  RISCV64_CSR_VLENB = 0xc22,
};

/**
 * The bits of sstatus.
 */
struct sstatus {
  union {
    u64 bits;
    struct {
      u64 pad0 : 1;
      u64 sie : 1;
      u64 pad1 : 3;
      u64 spie : 1;
      u64 ube : 1;
      u64 pad2 : 1;
      u64 spp : 1;
      u64 vs : 2;
      u64 pad3 : 2;
      u64 fs : 2;
      u64 xs : 2;
      u64 pad4 : 1;
      u64 sum : 1;
      u64 mxr : 1;
      u64 pad5 : 3;
      u64 spelp : 1;
      u64 sdt : 1;
      u64 pad6 : 7;
      u64 uxl : 2;
      u64 pad7 : 29;
      u64 sd : 1;
    };
  };
};

static_assert(sizeof(struct sstatus) == sizeof(u64));
static_assert(alignof(struct sstatus) == alignof(u64));

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
