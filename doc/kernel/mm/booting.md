# Booting

Before ordinary driver code can run, the three main allocators in the mm subsystem (heap, physical, virtual) need to be initialized.
This document describes that initialization.

## Build-time

At build-time, we can request pages (as `.bss` or `.data`) that we're given before the kernel starts, saving us from having to implement allocation that early in boot.
We use this to allocate initial page tables and some initial heap and stack memory.

See `src/kernel/arch/riscv64/generate_bootstub.py` for the code that does this.

The initial page tables are generated ahead-of-time and compiled into the kernel binary, so that virtual memory works immediately, before any allocators are up.
These page tables have entries for:

- The physical memory mapping.
  - The entire mapping is made, using 1GiB pages.
- The initial heap segment.
  - This is 4MiB, and gets mapped to the RAM area.
- The boothart's stack.
  - This is 2MiB, and gets mapped to the RAM area.
  - After the physical memory allocator is set up, guard pages get set up.
- The kernel.

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
- `/reserved-memory` nodes, which we avoid adding to the physical allocator.

From the memory and reservations, we can find all the free regions of unreserved RAM.
We use a simple free list to track them.

Once this is done, the heap allocator is able to allocate more heap segments from the physical allocator, so there's no longer a 4MiB limitation on heap allocation.

## Virtual allocator

The virtual allocator for higher-half memory can now be initialized as well.
This allocator covers the entire 38-bit space, but will only ever have the RAM area marked as free.
