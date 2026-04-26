/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <mm/alloc.h>
#include <mm/paging.h>
#include <mm/physical_alloc.h>
#include <scheduler.h>
#include <stdatomic.h>
#include <task.h>

/**
 * Switches to the given task.
 *
 * TODO: This should require that a hartlock be held.
 */
void task_switch(struct task *task);

struct task *task_new(struct hart *hart, void (*main)(void *), void *main_arg) {
  assert(hart);
  assert(hart->hart_group);

  // Allocate the task.
  struct task *task =
      alloc(sizeof(struct task) + hart->hart_group->sizeof_register_save_area);
  if (!task)
    return nullptr;
  struct vma *vma = vma_alloc(&kernel_virtual_allocator, 1 << 9);
  if (!vma) {
    free(task);
    return nullptr;
  }

  // Allocate the task's stack.
  uaddr stack_lo, stack_hi;
  vma_bounds(vma, &stack_lo, &stack_hi);
  if (!mm_alloc_and_map_many(stack_lo, stack_hi, PHYSICAL_ALLOC_DEFAULT,
                             PGPERM_KRW)) {
    vma_free(vma);
    free(task);
    TODO();
  }
  mm_paging_fence();

  // Initialize the task.
  struct task_priorities priorities = {
      .base = PRIORITY_DEFAULT,
      .effective = PRIORITY_DEFAULT,
  };
  *task = (struct task){
      .list = LIST_INIT(task->list),
      .flags = TASK_STATE_CONSTRUCTING,
      .hart_group = hart->hart_group,
      .stack_vma = vma,
  };
  atomic_init(&task->priorities, priorities);

  // Set up the registers to call the function.
  hart->hart_ops->set_task_regs(hart, task, main, main_arg);

  return task;
}

void task_yield() {
  // Ask the scheduler for another task to run. If there was nothing to run,
  // don't try to switch.
  struct task *new_task = scheduler_get();
  if (!new_task)
    return;
  assert(new_task->flags == TASK_STATE_RUNNING);

  // Get the current task.
  struct task *old_task = get_hart_locals()->task;

  // Hand the current task back to the scheduler.
  //
  // TODO: This _really_ ought to be under some kind of lock that gets released
  // during the context-switch.
  assert(old_task->flags == TASK_STATE_RUNNING);
  scheduler_put(old_task);
  assert(old_task->flags == TASK_STATE_RUNNABLE);

  // Switch to the new task.
  task_switch(new_task);

  // Make sure we came back all right.
  assert(get_hart_locals()->task == old_task);
  assert(old_task->flags == TASK_STATE_RUNNING);
}

[[noreturn]]
void task_exit() {
  TODO();
}
