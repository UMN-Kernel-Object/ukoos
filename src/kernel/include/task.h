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

// Avoids a circular include:
//   task.h -> devices/hart.h  for struct hart
//   devices/hart.h -> task.h  for struct task
struct hart;

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

/**
 * A type for priorities. All values 0-15 are valid.
 */
enum priority : u8 {
  PRIORITY_DEFAULT = 8,
  PRIORITY_USERSPACE_APPLICATION = 8,
  PRIORITY_USERSPACE_BACKGROUND = 4,
  PRIORITY_USERSPACE_UI = 11,
  PRIORITY_USERSPACE_MAX = 11,
  PRIORITY_USERSPACE_MIN = 4,
};

struct task_priorities {
  enum priority base : 4;
  enum priority effective : 4;
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
   * The hart group the task can run on.
   */
  struct hart_group *hart_group;

  /**
   * The VMA corresponding to the task's stack.
   */
  struct vma *stack_vma;

  /**
   * The register save area.
   */
  u8 register_save[];
};

// Check that the layout that's assumed by assembly code.
static_assert(offsetof(struct task, flags) == 16);
static_assert(offsetof(struct task, hart_group) == 24);
static_assert(offsetof(struct task, register_save) == 40);

/**
 * Allocates a new task struct that can run on the given hart and initializes
 * it to start execution at the given function.
 */
struct task *task_new(struct hart *hart, void (*main)(void *), void *main_arg);

/**
 * Switches to the given task.
 *
 * TODO: This should require that a hartlock be held.
 */
void task_switch(struct task *task);

#endif // UKO_OS_KERNEL__TASK_H
