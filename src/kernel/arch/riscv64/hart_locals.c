/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <arch/riscv64/insns.h>
#include <device.h>
#include <hart_locals.h>
#include <mm/alloc.h>
#include <stdatomic.h>
#include <task.h>

/**
 * Storage for the boothart's hart-locals.
 */
static struct hart_locals boothart_hart_locals;

/**
 * The boothart's hart ID.
 */
static u64 boothart_hart_id;

/**
 * Modifies a heap to change the hart that owns it. This is only needed when
 * initializing the boothart, so it's declared here instead of in a public
 * header.
 */
void heap_change_boothart_hart(struct mm_alloc_heap *heap, struct hart *hart);

struct hart_locals *get_hart_locals(void) {
  return (struct hart_locals *)csrr(RISCV64_CSR_SSCRATCH);
}

void init_boothart_hart_locals_early(u64 hart_id) {
  boothart_hart_id = hart_id;
  csrw(RISCV64_CSR_SSCRATCH, (u64)&boothart_hart_locals);
  boothart_hart_locals = (struct hart_locals){
      .hart = nullptr,
      .task = nullptr,
      .heap = nullptr,
      .rng = {},
  };
}

void init_boothart_hart_locals_late(struct vma *initial_stack_vma) {
  // Check that we're on the boothart.
  assert(get_hart_locals() == &boothart_hart_locals);

  // As a quick sanity-check, check that the current stack pointer is in the
  // stack VMA.
  uaddr lo, hi, sp;
  vma_bounds(initial_stack_vma, &lo, &hi);
  __asm__("mv %0, sp" : "=r"(sp)::);
  assert(lo <= sp && sp <= hi);

  // Reallocate the hart-locals to be heap-allocated.
  struct hart_locals *hart_locals = alloc(sizeof(struct hart_locals));
  memcpy(hart_locals, &boothart_hart_locals, sizeof(struct hart_locals));
  csrw(RISCV64_CSR_SSCRATCH, (u64)hart_locals);

  // Find the hart object with the current hart ID.
  for (struct list_head *hart_list = harts.next; hart_list != &harts;
       hart_list = hart_list->next) {
    struct hart *hart = container_of(hart_list, struct hart, list);
    if (hart->id == boothart_hart_id) {
      hart_locals->hart = hart;
      break;
    }
  }
  if (!hart_locals->hart) {
    print_devices();
    panic("Could not find a hart with the ID {u64}", boothart_hart_id);
  }

  // Fix up the heap to refer to this hart.
  heap_change_boothart_hart(hart_locals->heap, hart_locals->hart);

  // Ensure that the hart has a hart group.
  if (!hart_locals->hart->hart_group)
    hart_locals->hart->hart_ops->detect_hart_group(hart_locals->hart);
  assert(hart_locals->hart->hart_group);

  // Create a task object for the current task. This doesn't go through
  // task_new, because we don't want to e.g. allocate a stack for the new task.
  hart_locals->task =
      alloc(sizeof(struct task) +
            hart_locals->hart->hart_group->sizeof_register_save_area);
  assert(hart_locals->task);
  struct task_priorities priorities = {
      .base = PRIORITY_DEFAULT,
      .effective = PRIORITY_DEFAULT,
  };
  *hart_locals->task = (struct task){
      .list = LIST_INIT(hart_locals->task->list),
      .flags = TASK_STATE_RUNNING,
      .hart_group = hart_locals->hart->hart_group,
      .stack_vma = initial_stack_vma,
  };
  atomic_init(&hart_locals->task->priorities, priorities);

  // Put the task into the scheduler.
  TODO();
}

void init_hart_locals(u64 hart_id);
