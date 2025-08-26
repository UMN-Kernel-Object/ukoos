#ifndef UKO_OS_KERNEL__HARTLOCK_H
#define UKO_OS_KERNEL__HARTLOCK_H 1

/**
 * hart_lock - Locks the executing thread to the current hart.
 *
 * Disables interrupts to prevent the current thread from being moved
 * to another hart. This does not provide mutual exclusion, nor does it 
 * prevent the thread from blocking. It only ensures that the 
 * thread continues to execute on the same hart by preventing
 * preemption via interrupts.
 */
void hart_lock(void);

/**
 * hart_unlock - Unlocks the executing thread from the current hart.
 *
 * Re-enables interrupts if they were enabled at the time hart_lock()
 * was called. If interrupts were already disabled, they remain disabled.
 */
void hart_unlock(void);

#endif  // UKO_OS_KERNEL__HARTLOCK_H

