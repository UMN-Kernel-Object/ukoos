/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__HART_LOCALS_H
#define UKO_OS_KERNEL__HART_LOCALS_H 1

#include <crypto/subtle/random_internals.h>

struct riscv_regs {
  u64 ra;
  u64 sp;
  u64 gp;
  u64 tp;
  u64 t0;
  u64 t1;
  u64 t2;
  u64 s0;
  u64 s1;
  u64 a0;
  u64 a1;
  u64 a2;
  u64 a3;
  u64 a4;
  u64 a5;
  u64 a6;
  u64 a7;
  u64 s2;
  u64 s3;
  u64 s4;
  u64 s5;
  u64 s6;
  u64 s7;
  u64 s8;
  u64 s9;
  u64 s10;
  u64 s11;
  u64 t3;
  u64 t4;
  u64 t5;
  u64 t6;
};

/**
 * The data that is local to a hart.
 */
struct hart_locals {
  /**
   * The ID of the hart. Not guaranteed to be small or densely packed.
   */
  u64 hart_id;

  /**
   * The current heap.
   */
  struct mm_alloc_heap *heap;

  /**
   * The hart-local RNG.
   */
  struct random_rng rng;

  /*
   * Saved registers
   */
  struct riscv_regs regs;
};

/**
 * Returns a pointer to the current hart's locals.
 *
 * TODO: Make sure this is called under some lock that ensures the current
 * thread cannot be rescheduled. Otherwise, it'd be super-easy to get a race.
 */
struct hart_locals *get_hart_locals(void);

/**
 * Called during early boot to initialize the boothart's hart-local storage.
 * This needs to be separate from `init_hart_locals`, because the allocator
 * won't be available yet.
 */
void init_boothart_hart_locals(u64 hart_id);

/**
 * Called during hart bring-up to allocate hart-local storage for a hart.
 */
void init_hart_locals(u64 hart_id);

#endif // UKO_OS_KERNEL__HART_LOCALS_H
