/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

  // When acquiring a spinlock, disable local interrupts.
  // Otherwise, we might get interrupted and then attempt to re-acquire the same
  // spinlock, leading to a deadlock.
  u64 flags = local_irq_save();

  struct hart_locals *locals = get_hart_locals();
  u64 hart_id = locals->hart_id;

  if (spinlock->owner_hart_id == hart_id) {
    panic("hart {u64} tried to re-acquire spinlock it already owns "
          "from {cstr}:{usize} in {cstr}",
          hart_id, file, line, func);
  }

  bool expected = false;

  // Try to acquire the lock.
  while (!__atomic_compare_exchange_n(&spinlock->locked, &expected, true, false,
                                      __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
    expected = false;
    pause_hint();
  }

  spinlock->flags = flags;

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