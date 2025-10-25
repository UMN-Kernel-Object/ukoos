/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <align.h>
#include <arch/riscv64/insns.h>
#include <mm/paging.h>
#include <mm/physical_alloc.h>
#include <panic.h>

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
  u64 rsvd0 : 10;
};
static_assert(sizeof(struct pte) == 8);
static_assert(alignof(struct pte) == 8);

/**
 * An Sv39 page table.
 */
struct pgtbl {
  alignas(4096) struct pte ptes[512];
};
static_assert(sizeof(struct pgtbl) == 4096);
static_assert(alignof(struct pgtbl) == 4096);

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
static_assert(sizeof(struct satp) == 8);
static_assert(alignof(struct satp) == 8);

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

static bool mm_paging_walk(uaddr va, paddr *pte, bool alloc) {
  assert(is_aligned(va, 12), "va={uaddr}", va);
  assert((uaddr)(((iaddr)va >> 39) + 1) <= 1, "va={uaddr}", va);
  assert(pte);

  usize vpn2 = (va >> 30) & 0x1ff, vpn1 = (va >> 21) & 0x1ff,
        vpn0 = (va >> 12) & 0x1ff;
  paddr pgtbl2 = {0}, pgtbl1 = {0}, pgtbl0 = {0};
  paddr pte2_addr, pte1_addr, pte0_addr;
  struct pte pte2, pte1;

  struct satp satp = satp_read();
  if (satp.mode != SATP_MODE_SV39)
    TODO("support non-Sv39 paging");
  pgtbl2.ppn = satp.ppn;

  pte2_addr = paddr_offset(pgtbl2, vpn2 * sizeof(struct pte));
  copy_from_physical(&pte2, pte2_addr, sizeof(struct pte));
  if (!pte2.valid) {
    if (!alloc)
      return false;
    if (!mm_alloc_physical(&pgtbl1))
      return false;
    bzero_physical(pgtbl1, sizeof(struct pgtbl));
    pte2 = (struct pte){
        .valid = true,
        .ppn = pgtbl1.ppn,
    };
    copy_to_physical(pte2_addr, &pte2, sizeof(struct pte));
  }
  pgtbl1.ppn = pte2.ppn;

  pte1_addr = paddr_offset(pgtbl1, vpn1 * sizeof(struct pte));
  copy_from_physical(&pte1, pte1_addr, sizeof(struct pte));
  if (!pte1.valid) {
    if (!alloc)
      return false;
    if (!mm_alloc_physical(&pgtbl0))
      return false;
    bzero_physical(pgtbl0, sizeof(struct pgtbl));
    pte1 = (struct pte){
        .valid = true,
        .ppn = pgtbl0.ppn,
    };
    copy_to_physical(pte1_addr, &pte1, sizeof(struct pte));
  }
  pgtbl0.ppn = pte1.ppn;

  pte0_addr = paddr_offset(pgtbl0, vpn0 * sizeof(struct pte));
  *pte = pte0_addr;
  return true;
}

bool mm_paging_map(uaddr va, paddr pa, enum page_permissions perms) {
  assert(is_aligned(va, 12), "va={uaddr}", va);
  assert((uaddr)(((iaddr)va >> 39) + 1) <= 1, "va={uaddr}", va);
  assert(is_aligned(pa, 12), "pa={uaddr}", pa);

  paddr pte_addr;
  if (!mm_paging_walk(va, &pte_addr, true))
    return false;

  struct pte pte;
  copy_from_physical(&pte, pte_addr, sizeof(struct pte));
  assert(!pte.valid);

  pte = (struct pte){
      .valid = true,
      .readable = true,
      .accessed = true,
      .dirty = true,
      .ppn = pa.ppn,
  };
  switch (perms) {
  case PGPERM_KRO:
    pte.global = true;
    break;
  case PGPERM_KRW:
    pte.writable = true;
    pte.global = true;
    break;
  case PGPERM_KRX:
    pte.executable = true;
    pte.global = true;
    break;
  case PGPERM_URO:
    pte.user = true;
    break;
  case PGPERM_URW:
    pte.writable = true;
    pte.user = true;
    break;
  case PGPERM_URX:
    pte.executable = true;
    pte.user = true;
    break;
  }

  copy_to_physical(pte_addr, &pte, sizeof(struct pte));
  return true;
}

bool mm_paging_unmap(uaddr va, paddr *pa) {
  assert(is_aligned(va, 12), "va={uaddr}", va);
  assert((uaddr)(((iaddr)va >> 39) + 1) <= 1, "va={uaddr}", va);
  assert(pa);
  *pa = paddr_of_bits(0);

  paddr pte_addr;
  if (!mm_paging_walk(va, &pte_addr, false))
    return false;

  struct pte pte;
  copy_from_physical(&pte, pte_addr, sizeof(struct pte));
  if (!pte.valid)
    return false;

  pa->ppn = pte.ppn;
  pte = (struct pte){0};
  copy_to_physical(pte_addr, &pte, sizeof(struct pte));

  return true;
}
