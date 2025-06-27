#include <arch/riscv64/insns.h>
#include <endian.h>
#include <mm/paging.h>
#include <panic.h>
#include <physical.h>

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

static u64 bits_of_pte(struct pte pte) {
  union {
    u64 bits;
    struct pte pte;
  } u = {.pte = pte};
  return u.bits;
}

static struct pte pte_of_bits(u64 bits) {
  union {
    u64 bits;
    struct pte pte;
  } u = {.bits = bits};
  return u.pte;
}

/**
 * An Sv39 page table.
 */
struct pgtbl {
  struct pte ptes[512];
};

/**
 * The default root page table of the kernel.
 */
extern struct pgtbl kernel_pgtbl2;

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
  // TODO: Do we need to use OpenSBI to update this on every hart?
  sfence_vma();
}

/**
 * Flags for calling the walk function.
 */
enum walk_flags {
  WALK_ALLOC = (1 << 0),
};

/**
 * Returns the physical address of the PTE that maps the given address.
 *
 * If the page table containing the PTE does not exist:
 * - if `walk_flags` includes `WALK_ALLOC`, tries to allocate new page tables
 *   - on success, returns a pointer to the PTE
 *   - on failure, returns `nullptr`
 * - otherwise, returns `nullptr`
 */
paddr walk(uaddr vaddr, enum walk_flags flags) {
  assert((vaddr & 0xfff) == 0, "vaddr={uaddr}", vaddr);

  if (vaddr == 0)
    return (paddr){0};

  uaddr ppn0 = (vaddr >> 12) & 0x1ff, ppn1 = (vaddr >> 21) & 0x1ff,
        ppn2 = (vaddr >> 30) & 0x1ff;
  if (ppn2 & 0x100)
    assert(((iaddr)vaddr >> 39) == (iaddr)-1, "vaddr={uaddr}", vaddr);
  else
    assert(((iaddr)vaddr >> 39) == 0, "vaddr={uaddr}", vaddr);

  struct satp satp = satp_read();
  assert(satp.mode == SATP_MODE_SV39, "satp.mode={u64:#x}", (u64)satp.mode);

  paddr pgtbl2 = paddr_of_bits(satp.ppn << 12);
  if (!bits_of_paddr(pgtbl2))
    return (paddr){0};
  struct pte pte2 =
      pte_of_bits(physical_read_u64le(paddr_offset(pgtbl2, ppn2 << 3)));
  if (!pte2.valid) {
    if (flags & WALK_ALLOC)
      TODO();
    else
      return (paddr){0};
  }
  assert(!pte2.readable && !pte2.writable && !pte2.executable);

  paddr pgtbl1 = paddr_of_bits(pte2.ppn << 12);
  if (!bits_of_paddr(pgtbl1))
    return (paddr){0};
  struct pte pte1 =
      pte_of_bits(physical_read_u64le(paddr_offset(pgtbl1, ppn1 << 3)));
  if (!pte1.valid) {
    if (flags & WALK_ALLOC)
      TODO();
    else
      return (paddr){0};
  }
  assert(!pte1.readable && !pte1.writable && !pte1.executable);

  paddr pgtbl0 = paddr_of_bits(pte1.ppn << 12);
  if (!bits_of_paddr(pgtbl0))
    return (paddr){0};
  return paddr_offset(pgtbl0, ppn0 << 3);
}

bool _mm_map(uaddr va, paddr pa, enum page_permissions perms) {
  assert((va & 0xfff) == 0, "va={uaddr}", va);
  assert(!pa.offset, "pa={paddr}", pa);

  struct pte pte = {
      .ppn = pa.ppn,
      .dirty = true,
      .accessed = true,
      .global = false,
      .user = false,
      .executable = false,
      .writable = false,
      .readable = false,
      .valid = true,
  };
  switch (perms) {
  case PGPERM_KRO:
    pte.readable = true;
    break;
  case PGPERM_KRW:
    pte.readable = true;
    pte.writable = true;
    break;
  case PGPERM_KRX:
    pte.readable = true;
    pte.executable = true;
    break;
  case PGPERM_URO:
    pte.user = true;
    pte.readable = true;
    break;
  case PGPERM_URW:
    pte.user = true;
    pte.readable = true;
    pte.writable = true;
    break;
  case PGPERM_URX:
    pte.user = true;
    pte.readable = true;
    pte.executable = true;
    break;
  }

  paddr pte_addr = walk(va, WALK_ALLOC);
  if (!bits_of_paddr(pte_addr))
    return false;
  physical_write_u64le(pte_addr, bits_of_pte(pte));
  return true;
}

/**
 * The address of a page that `physical_{read,write}_u{8,{16,32,64}{l,b}e}` use
 * as a temporary.
 */
static constexpr uptr physical_tmp_page = 0xffffffff80000000;

struct superpage_paddr {
  unsigned long offset : 30;
  unsigned long hi_ppn : 26;
  unsigned long rsvd : 8;
};

static void *physical_map(paddr paddr) {
  assert(!paddr.rsvd, "paddr={paddr}", paddr);

  unsigned long offset = ((paddr.ppn & ((1 << 18) - 1)) << 12) + paddr.offset;
  kernel_pgtbl2.ptes[510] = (struct pte){
      .ppn = paddr.ppn & 0xfffc0000,
      .dirty = true,
      .accessed = true,
      .global = false,
      .user = false,
      .executable = false,
      .writable = true,
      .readable = true,
      .valid = true,
  };
  sfence_vma();

  return (void *)(physical_tmp_page + offset);
}

u8 physical_read_u8(paddr paddr) { return *(u8 *)physical_map(paddr); }

u16 physical_read_u16le(paddr paddr) {
  assert(!(paddr.offset & 0x1), "paddr={paddr}", paddr);
  return little_to_native(*(u16 *)physical_map(paddr));
}

u32 physical_read_u32le(paddr paddr) {
  assert(!(paddr.offset & 0x3), "paddr={paddr}", paddr);
  return little_to_native(*(u32 *)physical_map(paddr));
}

u64 physical_read_u64le(paddr paddr) {
  assert(!(paddr.offset & 0x7), "paddr={paddr}", paddr);
  return little_to_native(*(u64 *)physical_map(paddr));
}

u16 physical_read_u16be(paddr paddr) {
  assert(!(paddr.offset & 0x1), "paddr={paddr}", paddr);
  return big_to_native(*(u16 *)physical_map(paddr));
}

u32 physical_read_u32be(paddr paddr) {
  assert(!(paddr.offset & 0x3), "paddr={paddr}", paddr);
  return big_to_native(*(u32 *)physical_map(paddr));
}

u64 physical_read_u64be(paddr paddr) {
  assert(!(paddr.offset & 0x7), "paddr={paddr}", paddr);
  return big_to_native(*(u64 *)physical_map(paddr));
}

void physical_write_u8(paddr paddr, u8 value) {
  *(u8 *)physical_map(paddr) = value;
}

void physical_write_u16le(paddr paddr, u16 value) {
  assert(!(paddr.offset & 0x1), "paddr={paddr}", paddr);
  *(u16 *)physical_map(paddr) = native_to_little(value);
}

void physical_write_u32le(paddr paddr, u32 value) {
  assert(!(paddr.offset & 0x3), "paddr={paddr}", paddr);
  *(u32 *)physical_map(paddr) = native_to_little(value);
}

void physical_write_u64le(paddr paddr, u64 value) {
  assert(!(paddr.offset & 0x7), "paddr={paddr}", paddr);
  *(u64 *)physical_map(paddr) = native_to_little(value);
}

void physical_write_u16be(paddr paddr, u16 value) {
  assert(!(paddr.offset & 0x1), "paddr={paddr}", paddr);
  *(u16 *)physical_map(paddr) = native_to_big(value);
}

void physical_write_u32be(paddr paddr, u32 value) {
  assert(!(paddr.offset & 0x3), "paddr={paddr}", paddr);
  *(u32 *)physical_map(paddr) = native_to_big(value);
}

void physical_write_u64be(paddr paddr, u64 value) {
  assert(!(paddr.offset & 0x7), "paddr={paddr}", paddr);
  *(u64 *)physical_map(paddr) = native_to_big(value);
}

void copy_from_physical(void *dst, paddr src, usize len) {
  while (len) {
    const void *chunk = physical_map(src);
    usize chunk_len = 4096 - src.offset;
    if (chunk_len > len)
      chunk_len = len;
    memcpy(dst, chunk, chunk_len);
    src = paddr_offset(src, chunk_len);
    dst += chunk_len;
    len -= chunk_len;
  }
}

void copy_to_physical(paddr dst, const void *src, usize len) {
  while (len) {
    void *chunk = physical_map(dst);
    usize chunk_len = 4096 - dst.offset;
    if (chunk_len > len)
      chunk_len = len;
    memcpy(chunk, src, chunk_len);
    src += chunk_len;
    dst = paddr_offset(dst, chunk_len);
    len -= chunk_len;
  }
}
