# Memory map

ukoOS is a higher-half kernel (i.e., all the kernel's data is mapped to an address whose MSB is 1).
Depending on how many bits of virtual address space the hardware supports, the kernel memory map is somewhat different.

## Sv39 memory map

In Sv39, virtual addresses are 39 bits, and sign-extended to 64 bits.

| Start Address        | End Address          | Size           | Description               |
|:---------------------|:---------------------|:---------------|:--------------------------|
| `0x0000000000000000` | `0x0000003fffffffff` | 256GiB         | Userspace virtual memory  |
| `0x0000004000000000` | `0xffffffbfffffffff` | 16EiB - 512GiB | Illegal addresses in Sv39 |
| `0xffffffc000000000` | `0xffffffdfffffffff` | 128GiB         | Physical memory           |
| `0xffffffe000000000` | `0xffffffffbfffffff` | 127GiB         | RAM                       |
| `0xffffffffc0000000` | `0xffffffffffffffff` | 1GiB           | Kernel                    |

- The userspace memory map can be controlled from userspace, and does not have a fixed structure.

- A large range of 64-bit addresses are illegal in Sv39, because there are not enough bits to represent them.

- 128GiB of physical memory is directly mapped.
  This should be enough to access any memory-mapped devices; devices that use more physical memory than this tend to support Sv48 or Sv57.

- Up to 127GiB of RAM can be mapped.
  Past this point, no more memory can be used by the kernel.
  Machines with anywhere near this much memory support Sv48 or Sv57, so this isn't a limitation in practice.

  Memory gets mapped here by the allocator as needed.

- The kernel itself is mapped into a large contiguous region.
  There are a lot of smaller regions within this region, but they're outside the scope of this page.
