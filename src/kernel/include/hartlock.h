#ifndef UKO_OS_KERNEL__HARTLOCK_H
#define UKO_OS_KERNEL__HARTLOCK_H 1

/**
 * hartlock_acquire - Locks the executing thread to the current hart.
 *
 * Disables interrupts to prevent the current thread from being moved
 * to another hart. This does not provide mutual exclusion, nor does it 
 * prevent the thread from blocking. It only ensures that the 
 * thread continues to execute on the same hart by preventing
 * preemption via interrupts.
 */
void hartlock_acquire(void);

/**
 * hartlock_release - Unlocks the executing thread from the current hart.
 *
 * Re-enables interrupts if they were enabled at the time hartlock_acquire()
 * was called. If interrupts were already disabled, they remain disabled.
 */
void hartlock_release(void);

/**
 * is_hart_locked - Checks if the executing thread has a hartlock.
 *
 * Return: true if the thread has a hartlock, false otherwise.
 */
bool is_hart_locked(void);

#endif  // UKO_OS_KERNEL__HARTLOCK_H

