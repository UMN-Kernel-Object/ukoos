#include <hart_locals.h>
#include <hartlock.h>
#include <irq.h>
#include <panic.h>
#include <types.h>

void hartlock_acquire(void) {
  bool prev_interrupt_enabled = are_interrupts_enabled();
  disable_interrupts();
  struct hart_locals *hart = get_hart_locals();
  if (hart->interrupt_disabled_depth == 0) {
    hart->prev_interrupt_enabled = prev_interrupt_enabled;
  }
  hart->interrupt_disabled_depth += 1;
}

void hartlock_release(void) {
  if (are_interrupts_enabled()) {
    panic("hartlock_release: interrupts are enabled when they should be disabled");
  }
  struct hart_locals *hart = get_hart_locals();
  if (hart->interrupt_disabled_depth < 1) {
    panic("hartlock_release: called without matching hartlock_acquire");
  }
  hart->interrupt_disabled_depth -= 1;
  if (hart->interrupt_disabled_depth == 0 && hart->prev_interrupt_state) {
    enable_interrupts();
  }
}

bool is_hart_locked(void) {
  // Currently, a hartlock is just enabling/disabling interrupts
  return !are_interrupts_enabled();
}
