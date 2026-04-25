/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <scheduler.h>

/**
 * The list of running tasks, which serves as the owner of them all.
 */
struct list_head scheduler_running = LIST_INIT(scheduler_running);
