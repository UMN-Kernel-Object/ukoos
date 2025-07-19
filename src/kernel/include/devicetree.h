#ifndef UKO_OS_KERNEL__DEVICETREE_H
#define UKO_OS_KERNEL__DEVICETREE_H 1

#include <types.h>

/**
 * Registers the given DeviceTree globally.
 */
void devicetree_init(paddr devicetree_start);

/**
 * Uses the globally-registered DeviceTree to:
 * - find memory and give it to the physical allocator
 * - find an RNG seed and give it to the entropy pool
 *
 * Then, initializes the physical allocator, adjusting free_va_start and
 * free_va_end to account for the memory it allocates.
 */
void devicetree_mm_init(paddr kernel_start, paddr kernel_end,
                        uptr *free_va_start, uptr *free_va_end);

#endif // UKO_OS_KERNEL__DEVICETREE_H
