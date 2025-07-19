#ifndef UKO_OS_KERNEL__RANDOM_H
#define UKO_OS_KERNEL__RANDOM_H 1

#include <types.h>

/**
 * Initializes the global entropy pool. This must be called before adding
 * entropy to the pool.
 */
void entropy_pool_init(void);

/**
 * Adds entropy to the entropy pool without crediting it.
 */
__attribute__((access(read_only, 1, 2))) void entropy_pool_mix(const u8 *buf,
                                                               usize len);

/**
 * Fills `buf` with `len` random bytes.
 */
__attribute__((access(write_only, 1, 2))) void getrandom(u8 *buf, usize len);

/**
 * Returns a single random word.
 */
usize random(void);

/**
 * Blocks until the global entropy pool is initialized. If the local entropy
 * pool was initialized before the global entropy pool was, reinitializes the
 * local entropy pool.
 */
void wait_for_entropy(void);

#endif // UKO_OS_KERNEL__RANDOM_H
