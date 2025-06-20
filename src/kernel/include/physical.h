#ifndef UKO_OS_KERNEL__PHYSICAL_H
#define UKO_OS_KERNEL__PHYSICAL_H 1

/**
 * I/O to physical memory. The accesses performed by these functions are
 * volatile, and must be aligned.
 *
 * These functions are very inefficiently implemented -- drivers should map the
 * memory they needs to access instead. However, they do work before the memory
 * allocator is configured.
 */

#include <types.h>

u8 physical_read_u8(paddr);
u16 physical_read_u16le(paddr);
u32 physical_read_u32le(paddr);
u64 physical_read_u64le(paddr);
u16 physical_read_u16be(paddr);
u32 physical_read_u32be(paddr);
u64 physical_read_u64be(paddr);

void physical_write_u8(paddr, u8);
void physical_write_u16le(paddr, u16);
void physical_write_u32le(paddr, u32);
void physical_write_u64le(paddr, u64);
void physical_write_u16be(paddr, u16);
void physical_write_u32be(paddr, u32);
void physical_write_u64be(paddr, u64);

#endif // UKO_OS_KERNEL__PHYSICAL_H
