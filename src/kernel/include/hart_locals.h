/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__HART_LOCALS_H
#define UKO_OS_KERNEL__HART_LOCALS_H 1

#include <crypto/subtle/random_internals.h>
#include <devices/hart.h>
#include <hartlock.h>

/**
 * The data that is local to a hart.
 */
struct hart_locals {
  /**
   * A pointer to the current hart.
   */
  struct hart *hart;

  /**
   * The current task.
   */
  struct task *task;
  /**
   * The current heap.
   */
  struct mm_alloc_heap *heap;

  /**
   * The hart-local RNG.
   */
  struct random_rng rng;
};

// Check that the layout that's assumed by assembly code.
static_assert(offsetof(struct hart_locals, hart) == 0);
static_assert(offsetof(struct hart_locals, task) == 8);

/**
 * Returns a pointer to the current hart's locals.
 *
 * Requires a hartlock to be held. You can acquire a hartlock by using
 * `WITH_HARTLOCK` in a new scope; for example:
 *
 * ```c
 * void foo() {
 *   u8 blah = 2 + 2;
 *   {
 *     WITH_HARTLOCK(lock);
 *     struct hart_locals *locals = get_hart_locals(lock);
 *     
 *     // access `locals->rng`, etc ...
 *   }
 * }
 * ```
 */
struct hart_locals *get_hart_locals(struct hartlock *);

/**
 * Called during early boot to initialize the boothart's hart-local storage
 * enough to perform device discovery.
 */
void init_boothart_hart_locals_early(u64 hart_id);

/**
 * Called during early boot to finish initializing the boothart's hart-local
 * storage.
 */
void init_boothart_hart_locals_late(void);

/**
 * Called during hart bring-up to allocate hart-local storage for a hart.
 */
void init_hart_locals(u64 hart_id);

#endif // UKO_OS_KERNEL__HART_LOCALS_H
