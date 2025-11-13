#include <hartlock.h>
#include <panic.h>
#include <spinlock.h>

/* Returns true if thread is holding the specified spinlock, false otherwise. */
static bool holding(struct spinlock *sp) {
  struct hart_locals *hart = get_hart_locals();
  return atomic_load(&sp->lock) && (sp->hart == hart);
}

void spinlock_init(struct spinlock *sp, const char *name) {
  sp->lock = ATOMIC_FLAG_INIT;
  sp->hart = nullptr;
  sp->name = name;
}

void spinlock_acquire(struct spinlock *sp) {
  hartlock_acquire();  // Lock execution to current hart
  if (holding(sp)) {
    panic("spinlock_acquire: {cstr} already held by current thread", sp->name);
  }
  while (atomic_flag_test_and_set(&sp->lock, memory_order_acquire)) {}
  sp->hart = get_hart_locals();
}

void spinlock_release(struct spinlock *sp) {
  if (!holding(sp)) {
    panic("spinlock_release: {cstr} not held by current thread", sp->name);
  }
  sp->hart = nullptr;
  atomic_flag_clear_explicit(&sp->lock, memory_order_release);
  hartlock_release();  // Unlock execution if necessary
}
