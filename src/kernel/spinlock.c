/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdatomic.h>

#include <hart_locals.h>
#include <panic.h>
#include <spinlock.h>
#include <types.h>
// TODO: make generic across architectures
#include <arch/riscv64/insns.h>
#include <arch/riscv64/irq.h>

// None of these functions should be called manually.
// See doc/kernel/containers/guards.md for usage.

void __spinlock_guard_init(struct spinlock_guard *guard,
                           struct spinlock *spinlock, const char *file,
                           usize line, const char *func) {
  // Not really any reason this should happen in practice.
  if (spinlock == nullptr) {
    panic("in {cstr} from {cstr}:{usize}: passed a null spinlock", func, file,
          line);
  }

  // TODO: take a hartlock before this
  struct hart_locals *locals = get_hart_locals();
  u64 hart_id = locals->hart_id;

  if (spinlock->owner_hart_id == hart_id) {
    panic("hart {u64} tried to re-acquire spinlock it already owns "
          "from {cstr}:{usize} in {cstr}",
          hart_id, file, line, func);
  }

  bool expected = false;

  // Try to acquire the lock.
  while (!atomic_compare_exchange_weak_explicit(&spinlock->locked, &expected, true,
                                      memory_order_acquire, memory_order_relaxed)) {
    expected = false;
    pause_hint();
  }

  // Disable interrupts and save the previous interrupt state.
  // Otherwise, we may attempt to re-acquire the same lock from the same hart.
  spinlock->flags = local_irq_save();

  spinlock->owner_hart_id = hart_id;
  spinlock->file = file;
  spinlock->line = line;
  spinlock->func = func;

  guard->spinlock = spinlock;
}

void __spinlock_guard_free(struct spinlock_guard *guard) {
  if (guard == nullptr) {
    panic("spinlock guard free called with null guard");
  }

  struct spinlock *spinlock = guard->spinlock;
  if (spinlock == nullptr) {
    // TODO: warn here?
    return;
  }

  struct hart_locals *locals = get_hart_locals();
  u64 hart_id = locals->hart_id;

  if (spinlock->owner_hart_id != hart_id) {
    panic("hart {u64} tried to release spinlock owned by hart {u64} "
          "from {cstr}:{usize} in {cstr}",
          hart_id, spinlock->owner_hart_id, spinlock->file, spinlock->line,
          spinlock->func);
  }

  spinlock->owner_hart_id = 0;
  spinlock->file = nullptr;
  spinlock->line = 0;
  spinlock->func = nullptr;

  // Release the lock.
  u64 flags = spinlock->flags;
  guard->spinlock = nullptr;
  __atomic_store_n(&spinlock->locked, false, __ATOMIC_RELEASE);

  // Re-enable local interrupts.
  local_irq_restore(flags);
}
