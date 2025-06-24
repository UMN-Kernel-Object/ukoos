#include <arch/riscv64/insns.h>
#include <endian.h>
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
enum satp_mode {
  SATP_MODE_BARE = 0b0000,
  SATP_MODE_SV39 = 0b1000,
  SATP_MODE_SV48 = 0b1001,
  SATP_MODE_SV57 = 0b1010,
};

/**
 * A value to be stored in the SATP register.
 */
struct satp {
  enum satp_mode mode : 4;
  u16 asid : 16;
  u64 ppn : 44;
};

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

/**
 * Flags for calling the walk function.
 */
enum walk_flags {
  WALK_ALLOC = (1 << 0),
};

/**
 * Returns a pointer to the PTE that maps the given address.
 *
 * If the page table containing the PTE does not exist:
 * - if `walk_flags` includes `WALK_ALLOC`, tries to allocate new page tables
 *   - on success, returns a pointer to the PTE
 *   - on failure, returns `nullptr`
 * - otherwise, returns `nullptr`
 */
struct pte *walk(uptr vaddr, enum walk_flags flags) {
  assert((vaddr & 0xfff) == 0, "vaddr={uptr}", vaddr);

  // TODO
  return nullptr;
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

void copy_to_physical(paddr dst, const void *src, usize len);
