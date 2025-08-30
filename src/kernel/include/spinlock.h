#ifndef UKO_OS_KERNEL__SPINLOCK_H
#define UKO_OS_KERNEL__SPINLOCK_H 1

#include <hart_locals.h>
#include <types.h>

/**
 * struct spinlock - Basic spinlock structure.
 * @hart: Pointer to the hart holding the lock.
 * @name: Name of the lock (for debugging).
 * @state: Lock state (one if acquired, zero if free).
 */
struct spinlock {
	struct hart_locals *hart;
	const char *name;
	u32 state;
};

/**
 * initlock - Initializes a spinlock.
 * @sp: Pointer to the spinlock structure.
 * @name: Name of the lock.
 *
 * Initializes a spinlock that disables interrupts.
 */
void spinlock_init(struct spinlock *sp, const char *name);

/**
 * acquire - Acquire a spinlock.
 * @sp: Pointer to the spinlock structure.
 *
 * Acquires the specified spinlock. If the lock is already held,
 * the caller will spin until the lock becomes available.
 */
void spinlock_acquire(struct spinlock *sp);

/**
 * release - Release a spinlock.
 * @sp: Pointer to the spinlock structure.
 *
 * Releases the previously acquired spinlock. The lock must have
 * been acquired by the same context.
 */
void spinlock_release(struct spinlock *sp);

#endif // UKO_OS_KERNEL__SPINLOCK_H

