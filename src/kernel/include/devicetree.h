#ifndef UKO_OS_KERNEL__DEVICETREE_H
#define UKO_OS_KERNEL__DEVICETREE_H 1

#include <types.h>

/**
 * Registers the given DeviceTree globally.
 */
void devicetree_init(paddr devicetree_start);

/**
 * Uses the globally-registered DeviceTree to find memory and give it to the
 * allocator.
 */
void devicetree_mm_init(paddr kernel_start, paddr kernel_end);

#endif // UKO_OS_KERNEL__DEVICETREE_H
