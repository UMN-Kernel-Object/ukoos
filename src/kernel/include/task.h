/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__TASK_H
#define UKO_OS_KERNEL__TASK_H 1

#include <mm/virtual_alloc.h>

/**
 * Kernel tasks and task switching.
 */

enum task_flags : u32 {
  /**
   * The task is not yet fully constructed. A task in a list should never be in
   * this state.
   */
  TASK_STATE_CONSTRUCTING = 0 << 0,

  /**
   * The task is currently running on a hart.
   */
  TASK_STATE_RUNNING = 1 << 0,

  /**
   * The task could be scheduled, but is not currently scheduled.
   */
  TASK_STATE_RUNNABLE = 2 << 0,

  /**
   * The task is waiting for an IO operation. A task in this state may be
   * spuriously woken up.
   */
  TASK_STATE_BLOCKED = 3 << 0,
};

struct task_priorities {
  u8 base : 4;
  u8 effective : 4;
};

static_assert(sizeof(struct task_priorities) == 1);
static_assert(alignof(struct task_priorities) == 1);

struct task {
  /**
   * The list of tasks this task is in.
   */
  struct list_head list;

  /**
   * The task's flags.
   */
  atomic enum task_flags flags;

  /**
   * The task's priorities.
   */
  atomic struct task_priorities priorities;

  /**
   * The VMA corresponding to the task's stack.
   */
  struct vma *stack_vma;

  /**
   * The size of the register save area, including any padding needed for
   * alignment.
   */
  usize register_save_size;

  /**
   * The register save area.
   *
   * TODO: Add a [[gnu::counted_by(register_save_size)]] once we're on GCC 15.
   */
  u8 register_save[];
};

#endif // UKO_OS_KERNEL__TASK_H
