#ifndef UKO_OS_KERNEL__HARTLOCK_H
#define UKO_OS_KERNEL__HARTLOCK_H 1

/**
 * Prevents thread from being moved to another hart, without preventing
 * the thread from blocking or providing mutual exclusion. It simply
 * disables interrupts.
 */
hart_lock();
  
/**
 * Releases hartlock by enabling interrupts if they were enabled when
 * hart_lock() was called. Otherwise, interrupts remain disabled.
 */
hart_unlock();

#endif  // UKO_OS_KERNEL__HARTLOCK_H

