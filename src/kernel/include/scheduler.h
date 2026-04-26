/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__SCHEDULER_H
#define UKO_OS_KERNEL__SCHEDULER_H 1

#include <task.h>

/**
 * Gets the next task this hart should run.
 *
 * Ensures:
 *
 * - If the task is non-`nullptr`, the task is in the list of running tasks.
 */
struct task *scheduler_get();

/**
 * Puts a task back in the scheduler, allowing other harts to run it later.
 *
 * Requires:
 *
 * - The task is in the list of running tasks.
 */
void scheduler_put(struct task *task);

/**
 * Puts a new task into the scheduler's list of runnable tasks.
 *
 * Requires:
 *
 * - `list_is_empty(&task->list)`
 * - `task->flags == TASK_STATE_CONSTRUCTING`
 */
void scheduler_put_new(struct task *task);

#endif // UKO_OS_KERNEL__SCHEDULER_H
