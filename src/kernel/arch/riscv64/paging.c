/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <arch/riscv64/insns.h>

/**
 * An Sv39 page table entry.
 */
struct pte {
  bool valid : 1;
  bool readable : 1;
  bool writable : 1;
  bool executable : 1;
  bool user : 1;
  bool global : 1;
  bool accessed : 1;
  bool dirty : 1;
  u64 rsvd1 : 2;
  u64 ppn : 44;
  u64 rsrvd0 : 10;
};

/**
 * An Sv39 page table.
 */
struct pgtbl {
  struct pte ptes[512];
};

/**
 * The MODE field of the SATP CSR.
 */
enum satp_mode : u64 {
  SATP_MODE_BARE = 0b0000,
  SATP_MODE_SV39 = 0b1000,
  SATP_MODE_SV48 = 0b1001,
  SATP_MODE_SV57 = 0b1010,
};

/**
 * A value to be stored in the SATP register.
 */
struct satp {
  u64 ppn : 44;
  u64 asid : 16;
  enum satp_mode mode : 4;
};
static_assert(sizeof(struct satp) == sizeof(u64));

/**
 * Reads from SATP.
 */
struct satp satp_read(void) {
  union {
    u64 bits;
    struct satp satp;
  } u;
  u.bits = csrr(RISCV64_CSR_SATP);
  return u.satp;
}

/**
 * Writes to SATP.
 */
void satp_write(struct satp satp) {
  union {
    u64 bits;
    struct satp satp;
  } u;
  u.satp = satp;
  csrw(RISCV64_CSR_SATP, u.bits);
}

void mm_paging_fence(void) {
  // TODO: Do a TLB shootdown if necessary.
  sfence_vma();
}
