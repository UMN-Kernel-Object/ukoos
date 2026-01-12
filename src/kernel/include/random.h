/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__RANDOM_H
#define UKO_OS_KERNEL__RANDOM_H 1

#include <types.h>

/**
 * Initializes the global entropy pool. This must be called before adding
 * entropy to the pool.
 */
void entropy_pool_init(void);

/**
 * Credits the entropy pool.
 */
void entropy_pool_credit(usize bits);

/**
 * Adds entropy to the entropy pool without crediting it.
 */
[[gnu::access(read_only, 1, 2)]] void entropy_pool_mix(const u8 *buf,
                                                       usize len);

/**
 * Seeds the global entropy pool with entropy that can be added very early in
 * boot. What and how much this is depends heavily on the architecture, and may
 * vary from device to device.
 */
void arch_entropy_pool_seed_early(void);

/**
 * Fills `buf` with `len` random bytes.
 */
[[gnu::access(write_only, 1, 2)]] void getrandom(u8 *buf, usize len);

/**
 * Returns a single random word.
 */
usize random(void);

/**
 * Returns a single random u32.
 */
u32 random_u32(void);

/**
 * Returns a single random u64.
 */
u64 random_u64(void);

/**
 * Blocks until the global entropy pool is initialized. If the local entropy
 * pool was initialized before the global entropy pool was, reinitializes the
 * local entropy pool.
 */
void wait_for_entropy(void);

#endif // UKO_OS_KERNEL__RANDOM_H
