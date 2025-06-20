#include <arch/riscv64/insns.h>
#include <panic.h>
#include <physical.h>

/**
 * An Sv39 page table entry.
 */
struct pte {
  u64 rsrvd0 : 10;
  u64 ppn : 44;
  u64 rsvd1 : 2;
  bool dirty : 1;
  bool accessed : 1;
  bool global : 1;
  bool user : 1;
  bool executable : 1;
  bool writable : 1;
  bool readable : 1;
  bool valid : 1;
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
 * Returns a pointer to the PTE the maps the given address.
 *
 * If the page table containing the PTE does not exist:
 * - if `walk_flags` includes `WALK_ALLOC`, tries to allocate new page tables
 *   - on success, returns a pointer to the PTE
 *   - on failure, returns `nullptr`
 * - otherwise, returns `nullptr`
 */
struct pte *walk(uptr vaddr, enum walk_flags flags) {
  assert((vaddr & 0xfff) == 0);

  // TODO
  return nullptr;
}

/**
 * The address of a page that `physical_{read,write}_u{8,{16,32,64}{l,b}e}` use
 * as a temporary.
 */
static constexpr uptr physical_tmp_page = 0xffffffff80000000;

u8 physical_read_u8(paddr paddr);
u16 physical_read_u16le(paddr);
u32 physical_read_u32le(paddr);
u64 physical_read_u64le(paddr);
u16 physical_read_u16be(paddr);
u32 physical_read_u32be(paddr);
u64 physical_read_u64be(paddr);

void physical_write_u8(paddr, u8) {
  struct pte *pte = walk(physical_tmp_page, 0);
  assert(pte);

  todo();
}

void physical_write_u16le(paddr, u16);
void physical_write_u32le(paddr, u32);
void physical_write_u64le(paddr, u64);
void physical_write_u16be(paddr, u16);
void physical_write_u32be(paddr, u32);
void physical_write_u64be(paddr, u64);
