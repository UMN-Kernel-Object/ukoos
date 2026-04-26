/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <mm/alloc.h>
#include <mm/paging.h>
#include <mm/physical_alloc.h>
#include <stdatomic.h>
#include <task.h>

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
