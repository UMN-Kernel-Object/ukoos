/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdatomic.h>

#include <hart_locals.h>
#include <panic.h>
#include <hartlock.h>
#include <types.h>
// TODO: make generic across architectures
#include <arch/riscv64/irq.h>

// None of these functions should be called manually.
// See doc/kernel/containers/guards.md for usage.

void __hartlock_init(struct hartlock *lock) {
  if(lock == nullptr) {
    panic("hartlock init called with null lock");
  }

  u64 flags = local_irq_save();

  lock->flags = flags;
}

void __hartlock_free(struct hartlock *lock) {
  if (lock == nullptr) {
    panic("hartlock free called with null lock");
  }

  local_irq_restore(lock->flags);
}
