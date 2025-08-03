#ifndef UKO_OS_KERNEL__PRINT_H
#define UKO_OS_KERNEL__PRINT_H 1

#include <mm/alloc.h>

/**
 * Formats a string as `print` does, saving the output to a (null-terminated)
 * string and returns it. Returns `nullptr` if there was not enough memory. This
 * string should be freed with `free`.
 */
[[gnu::malloc(free, 1), gnu::warn_unused_result]]
char *format(const char *fmt, ...);

/**
 * Prints a string to the kernel logs.
 */
void print(const char *fmt, ...);

#endif // UKO_OS_KERNEL__PRINT_H
