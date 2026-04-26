/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <scheduler.h>

/**
 * The list of running tasks.
 *
 * Not static, so that init_boothart_hart_locals_late can use it. Nothing else
 * should use it.
 */
struct list_head scheduler_running = LIST_INIT(scheduler_running);

/**
 * The list of runnable tasks.
 */
static struct list_head scheduler_runnable = LIST_INIT(scheduler_runnable);

/**
 * The list of blocked tasks.
 */
static struct list_head scheduler_blocked = LIST_INIT(scheduler_blocked);

void scheduler_put_new(struct task *task) {
  assert(list_is_empty(&task->list));
  assert(task->flags == TASK_STATE_CONSTRUCTING);

  task->flags = TASK_STATE_RUNNABLE;
  list_push(&scheduler_runnable, &task->list);
}
