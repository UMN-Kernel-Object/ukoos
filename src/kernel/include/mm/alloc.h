#ifndef UKO_OS_KERNEL__MM_ALLOC_H
#define UKO_OS_KERNEL__MM_ALLOC_H 1

#include <hart_locals.h>

/**
 * Frees memory returned by a previous call to `alloc`.
 *
 * When passed `nullptr`, does nothing. This makes code using
 * `__attribute__((cleanup))` easier to write.
 */
void free(void *ptr);

/**
 * Allocates `size` bytes of memory and returns a pointer to it. On OOM, returns
 * `nullptr`.
 *
 * - if `size` is greater than equal to 1 and less than or equal to 64, the
 *   returned pointer will be aligned to at least `stdc_bit_ceil(size)`.
 * - if `size` is greater than 64, the returned pointer will be aligned to at
 *   least 64.
 */
__attribute__((alloc_size(1), malloc, malloc(free, 1),
               warn_unused_result)) void *
alloc(usize size);

/**
 * Allocates zeroed-out memory. See `alloc` for the other conditions of this
 * function.
 */
__attribute__((alloc_size(1), malloc, malloc(free, 1),
               warn_unused_result)) void *
zalloc(usize size);

/**
 * Allocates memory, initializing it with the contents of `ptr`, which is then
 * `free`d. On OOM, returns `nullptr` and does *not* free `ptr`.
 *
 * - if `new_size` is greater than equal to 1 and less than or equal to 64, the
 *   returned pointer will be aligned to at least `stdc_bit_ceil(new_size)`.
 * - if `new_size` is greater than 64, the returned pointer will be aligned to
 *   at least 64.
 */
__attribute__((alloc_size(2), warn_unused_result)) void *
realloc(void *ptr, usize new_size);

#endif // UKO_OS_KERNEL__MM_ALLOC_H
