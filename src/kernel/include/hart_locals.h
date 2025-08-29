#ifndef UKO_OS_KERNEL__HART_LOCALS_H
#define UKO_OS_KERNEL__HART_LOCALS_H 1

#include <crypto/subtle/random_internals.h>
#include <mm/alloc_internals.h>

/**
 * The data that is local to a hart.
 */
struct hart_locals {
  /**
   * The ID of the hart. Not guaranteed to be small or densely packed.
   */
  u64 hart_id;
  
  /**
   * Number of times interrupts have been disabled.
   */
  u32 noff;

  /**
   * Were interrrupts enabled before noff nesting?
   */
  u32 intena;

  /**
   * The current heap.
   */
  struct mm_alloc_heap *heap;

  /**
   * The hart-local RNG.
   */
  struct random_rng rng;
};

/**
 * Returns a pointer to the current hart's locals.
 *
 * TODO: Make sure this is called under some lock that ensures the current
 * thread cannot be rescheduled. Otherwise, it'd be super-easy to get a race.
 */
struct hart_locals *get_hart_locals(void);

/**
 * Called during early boot to initialize the boothart's hart-local storage.
 * This needs to be separate from `init_hart_locals`, because the allocator
 * won't be available yet.
 */
void init_boothart_hart_locals(u64 hart_id);

/**
 * Called during hart bring-up to allocate hart-local storage for a hart.
 */
void init_hart_locals(u64 hart_id);

#endif // UKO_OS_KERNEL__HART_LOCALS_H
