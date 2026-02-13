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
.global switch_to_task
.type switch_to_task, @function
switch_to_task:
	ebreak
	ret
