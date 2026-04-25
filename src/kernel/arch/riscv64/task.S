/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

.section .text

/**
 * Switches tasks.
 */
.p2align 2
.global task_switch
.type task_switch, @function
task_switch:
	ebreak
	ret
