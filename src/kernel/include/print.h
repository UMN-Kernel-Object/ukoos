/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__PRINT_H
#define UKO_OS_KERNEL__PRINT_H 1

#include <mm/alloc.h>

/**
 * Formats a string as `print` does. This is not the same as GNU/BSD asprintf(),
 * and has different format strings! See "The print() and format() functions" in
 * the documentation.
 *
 * The returned string is a null-terminated string, which should be freed with
 * `free`. Returns `nullptr` if there was not enough memory.
 */
ATTR_FREE_WITH(free, 1)
[[gnu::warn_unused_result]]
char *format(const char *fmt, ...);

/**
 * Prints a string to the kernel logs. This is not the same as standard C
 * printf(), and has different format strings! See "The print() and format()
 * functions" in the documentation.
 */
void print(const char *fmt, ...);

#endif // UKO_OS_KERNEL__PRINT_H
