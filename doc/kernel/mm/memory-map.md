# Memory map

ukoOS is a higher-half kernel (i.e., all the kernel's data is mapped to an address whose MSB is 1).
Depending on how many bits of virtual address space the hardware supports, the kernel memory map is somewhat different.

## Sv39 memory map

In Sv39, virtual addresses are 39 bits, and sign-extended to 64 bits.

| Start Address        | End Address          | Size           | Description               |
|:---------------------|:---------------------|:---------------|:--------------------------|
| `0x0000000000000000` | `0x0000003fffffffff` | 256GiB         | Userspace virtual memory  |
| `0x0000004000000000` | `0xffffffbfffffffff` | 16EiB - 512GiB | Illegal addresses in Sv39 |
| `0xffffffc000000000` | `0xffffffdfffffffff` | 128GiB         | RAM                       |
| `0xffffffe000000000` | `0xffffffff3fffffff` | 125GiB         | Mappable memory           |
| `0xffffffff40000000` | `0xffffffff7fffffff` | 1GiB           | `physical_*` mapping      |
| `0xffffffff80000000` | `0xffffffffbfffffff` | 1GiB           | Kernel                    |
| `0xffffffffc0000000` | `0xffffffffffffffff` | 1GiB           | Page tables               |

- The userspace memory map can be controlled from userspace, and does not have a fixed structure.

- A large range of 64-bit addresses are illegal in Sv39, because there are not enough bits to represent them.

- Up to 128GiB of RAM can be mapped.
  Past this point, no more memory can be used.
  Machines with anywhere near this much memory support Sv48 or Sv57, so this isn't a limitation in practice.

  All the usable memory is mapped into a single contiguous range, so the allocator does not need to create new page table entries to allocate memory.

- A large range of addresses are treated as mappable memory.
  The virtual memory allocator allocates into this range of addresses.

  Drivers for memory-mapped devices map the devices into this range, and kthread stacks are also allocated into this range.

- The `physical_*` functions (`physical_read_*`, `physical_write_*`, `copy_from_physical`, `copy_to_physical`) work by mapping a 1GiB huge page into the address space and accessing it.
  These functions need to run very early in boot, so they can't allocate page tables.

- The kernel itself is mapped into a large contiguous region.
  There are a lot of smaller regions within this region, but they're outside the scope of this document.

- The current page tables are mapped into memory at a fixed offset, so that they can be accessed in a consistent place.
  This technique is called **recursive mapping**.
  We slightly abuse notation and call this 1GiB of virtual address space "the recursive mapping" as well.
  With recursive mapping, the root page table's last entry points to its _own_ physical address.

  This lets us access the page tables themselves as mapped memory at addresses in this mapping.
  Sv39 has 3 layers of page tables.
  The page table entries for a given virtual address can be found with the following equations:

  - `(((iaddr)addr >> 39) + 1) <= 1` must be true for the virtual address to be valid.
  - The level 2 PTE is at `0xffffffffc0000000 + (((addr >> 12) & ((1 << 27) - 1)) << 3)`.
  - The level 1 PTE is at `0xffffffffffe00000 + (((addr >> 21) & ((1 << 18) - 1)) << 3)`.
  - The level 0 PTE is at `0xfffffffffffff000 + (((addr >> 30) & ((1 <<  9) - 1)) << 3)`.

  Because 2MiB and 1GiB pages exist, not all of these PTEs necessarily exist, even when a virtual address is accessible.
  In particular, note that the level 1 and level 2 PTEs for the recursive mapping itself are not in the right regions overlap with other levels of PTEs.
