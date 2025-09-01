#include <hart_locals.h>
#include <hartlock.h>
#include <irq.h>
#include <panic.h>
#include <types.h>

void hartlock_acquire(void) {
	bool prev_irq_enabled = are_irq_enabled();
	irq_disable();
	struct hart_locals *hart = get_hart_locals();
	if (hart->irq_nesting_depth == 0) {
		hart->prev_irq_enabled = prev_irq_enabled;
	}
	hart->irq_nesting_depth += 1;
}

void hartlock_release(void) {
	if (are_irq_enabled()) {
		panic("hartlock_release: interrupts are enabled when they should be disabled");
	}
	struct hart_locals *hart = get_hart_locals();
	if (hart->irq_nesting_depth < 1) {
		panic("hartlock_release: called without matching hartlock_acquire");
	}
	hart->irq_nesting_depth -= 1;
	if (hart->irq_nesting_depth == 0 && hart->prev_irq_enabled) {
		irq_enable();
	}
}

bool is_hart_locked(void) {
	// Currently, a hartlock is just enabling/disabling interrupts
	return !are_irq_enabled();
}

