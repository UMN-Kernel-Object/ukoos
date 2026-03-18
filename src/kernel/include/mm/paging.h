/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__MM_PAGING_H
#define UKO_OS_KERNEL__MM_PAGING_H 1

#include <types.h>

/**
 * A fence on paging-related changes. The other functions declared in this
 * header do not necessarily take effect immediately -- call this function to
 * wait until they have taken effect.
 */
void mm_paging_fence(void);

/**
 * Permissions that can be given to pages.
 */
enum page_permissions : u8 {
  /**
   * A read-only page only accessible to the kernel.
   */
  PGPERM_KRO,

  /**
   * A read-write page only accessible to the kernel.
   */
  PGPERM_KRW,

  /**
   * An executable page only accessible to the kernel.
   */
  PGPERM_KRX,

  /**
   * A read-only page accessible to userspace. On some platforms, this will not
   * be accessible to the kernel by default.
   */
  PGPERM_URO,

  /**
   * A read-write page accessible to userspace. On some platforms, this will not
   * be accessible to the kernel by default.
   */
  PGPERM_URW,

  /**
   * An executable page accessible to userspace. On some platforms, this will
   * not be accessible to the kernel by default.
   */
  PGPERM_URX,
};

/**
 * Maps a physical page to a virtual address.
 *
 * Returns whether it succeeded.
 *
 * This function may not take effect immediately -- call `mm_paging_fence` to
 * wait until it has taken effect.
 */
bool mm_paging_map(uaddr va, paddr pa, enum page_permissions perms);

/**
 * Unmaps a physical page from a virtual address.
 *
 * Returns whether it succeeded, and writes the physical address to `*pa`.
 *
 * This function may not take effect immediately -- call `mm_paging_fence` to
 * wait until it has taken effect.
 */
bool mm_paging_unmap(uaddr va, paddr *pa);

/**
 * Maps a region of memory for memory-mapped IO. This may involve special flags
 * or other setup to prevent accesses from being cached.
 *
 * Returns a pointer to the virtual address for the region. Returns `nullptr` on
 * error.
 */
void *iomem_map(paddr addr, usize size);

/**
 * Unmaps memory mapped with `iomem_map`.
 */
void iomem_unmap(void *ptr, usize size);

#endif // UKO_OS_KERNEL__MM_PAGING_H
