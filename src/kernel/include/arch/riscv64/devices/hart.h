/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__ARCH_RISCV64_DEVICES_HART_H
#define UKO_OS_KERNEL__ARCH_RISCV64_DEVICES_HART_H 1

#include <devices/hart.h>

enum riscv_flen {
  /**
   * The F extension is not supported on this hart.
   */
  FLEN_0,

  /**
   * The F extension is supported on this hart, but the D and Q extensions are
   * not.
   */
  FLEN_32,

  /**
   * The F and D extensions are supported on this hart, but the Q extension is
   * not.
   */
  FLEN_64,

  /**
   * The F, D, and Q extensions are supported on this hart.
   */
  FLEN_128,
};

enum riscv_vlen {
  /**
   * The V extension is not supported on this hart.
   */
  VLEN_0,

  /**
   * This hart supports the V extension, but the VLEN of this hart is unknown
   * until it boots (which it hasn't yet done).
   */
  VLEN_UNKNOWN,

  VLEN_128,
  VLEN_256,
  VLEN_512,
  VLEN_1024,
  VLEN_2048,
  VLEN_4096,
  VLEN_8192,
  VLEN_16384,
  VLEN_32768,
  VLEN_65536,
};

/**
 * The key used to identify hart groups.
 *
 * This has to be small enough to be atomically updated thanks to the
 * VLEN_UNKNOWN state -- we can't guarantee other harts aren't accessing it
 * when the hart tries to take itself out of that state, so we need to use an
 * atomic write to update it.
 */
struct riscv_hart_group_key {
  /**
   * If the Zicboz extension is supported, the size of a cache block. Otherwise,
   * zero.
   */
  u32 cboz_block_size;
  enum riscv_flen flen : 2;
  enum riscv_vlen vlen : 4;
  // TODO: Sstateen-based extensions
};

static_assert(sizeof(struct riscv_hart_group_key) <= sizeof(u64));

/**
 * Returns whether the riscv_hart_group_key has been filled out enough to use to
 * find a hart group. This must be true before the hart enters the scheduler for
 * the first time.
 */
static inline bool
riscv_hart_group_key_is_fully_determined(struct riscv_hart_group_key key) {
  return key.vlen != VLEN_UNKNOWN;
}

/**
 * A hart group, plus some riscv64-specific extensions.
 */
struct riscv_hart_group {
  struct hart_group hart_group;

  // The offset and lengths in the register save area of the classes of GPRs.
  usize x_off, f_off, v_off;
  usize x_len, f_len, v_len;
};

/**
 * Finds or creates a hart group for the given key.
 *
 * Panics if the key is not fully determined.
 */
struct riscv_hart_group *riscv_hart_group_get(struct riscv_hart_group_key key);

#endif // UKO_OS_KERNEL__ARCH_RISCV64_DEVICES_HART_H
