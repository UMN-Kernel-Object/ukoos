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
 * - The task is in the list of running tasks.
 */
struct task *scheduler_get(void);

/**
 * Puts a task back in the scheduler, allowing other harts to run it later. The
 * task must not be accessed again after calling this function.
 *
 * Requires:
 *
 * - The task is in the list of running tasks.
 */
void scheduler_put(struct task *task);

/**
 * Puts a new task into the scheduler's list of runnable tasks. The task must
 * not be accessed again afer calling this function.
 *
 * Requires:
 *
 * - `list_is_empty(&task->list)`
 * - `task->flags == TASK_STATE_CONSTRUCTING`
 */
void scheduler_put_new(struct task *task);

#endif // UKO_OS_KERNEL__SCHEDULER_H
