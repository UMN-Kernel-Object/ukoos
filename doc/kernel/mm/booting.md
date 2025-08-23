# Booting

Before ordinary driver code can run, the three main allocators in the mm subsystem (heap, physical, virtual) need to be initialized.
This document describes that initialization.

## Build-time

At build-time, we can request pages (as `.bss` or `.data`) that we're given before the kernel starts, saving us from having to implement allocation that early in boot.
We use this to allocate initial page tables and some initial heap and stack memory.

The initial page tables are generated ahead-of-time and compiled into the kernel binary, so that virtual memory works immediately, before any allocators are up.
These page tables have entries for:

- The kernel.
- The initial heap segment.
  - This gets allocated as 4MiB of `.bss`.
  - This is mapped to the RAM area.
- The first two pages of the boothart's stack.
  - This gets allocated as 8KiB of `.bss`.
  - Each page has a guard page beneath it.
  - The stack grows automatically, but the page fault handler for doing so requires its own stack, hence the extra page.
  - This is mapped to both the RAM area (without the guard pages) and the mappable area (with them).
- The page tables themselves.
  - These get mapped to the RAM area, and are also reachable through the recursive mapping.
- The recursive mapping.

## Heap allocator

There is one instance of the heap allocator per hart, to avoid the need to acquire locks or use atomics in the fast-path of allocation.
The boothart's heap allocator gets initialized with the initial heap segment allocated at build-time.
This lets the boothart allocate up to 4MiB of objects whose sizes are less than 512KiB.

One annoying thing -- the heap allocator depends on a source of entropy.
This early in boot, the entropy pool cannot be fully seeded, so we have to try to harvest a bit of entropy to use.
Right now, our only source of entropy at this point is the cycle and time counters.
We take a trap while moving to the higher half, so we can get some unpredictability from the timings there; when booting on real hardware, the time taken to load the kernel from storage should also provide some.

## Physical allocator

Once the heap allocator is initialized on the boothart, we can discover the rest of the RAM.
We do this by parsing the Devicetree that was passed to us by the bootloader.

Once it's parsed, we can easily extract the parts of it we need:

- The `/chosen/rng-seed` node, as entropy to further initialize the entropy pool.
  This is usually enough to fully initialize the pool. 
- `/memory` nodes, which describe the memory installed on the device.
- Memory reservations to avoid, from the memory reservation block.
- `/reserved-memory` nodes, which must also be avoided.

From the memory and reservations, we can find all the free regions of RAM.
We use a buddy allocator to track them and map them into the RAM area.

There's a tiny bit of bootstrapping trickiness required, since making a mapping can require allocating from the physical allocator as well.
If there's not enough memory to make a page table mapping for a page we're adding, then instead we take that page to allocate a page table, adding it to the buddy allocator as being in-use instead of free.

Once this is done, the heap allocator is able to allocate more heap segments from it, lifting the 4MiB limitation.

## Virtual allocator

The virtual allocator for the mappable area can now be initialized as well.
This is much simpler, since we don't immediately need to perform any allocations, and the allocator does not need to describe itself -- the unmapped parts of the mappable area are the only thing we need to store in it, and its state lives entirely in heap-allocated data structures.
