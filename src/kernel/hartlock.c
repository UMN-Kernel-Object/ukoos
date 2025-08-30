#include <hart_locals.h>
#include <hartlock.h>
#include <irq.h>
#include <panic.h>
#include <types.h>

void hartlock_acquire(void) {
	u32 prev_irq_state = get_irq_state();
	irq_disable();
	struct hart_locals *hart = get_hart_locals();
	if (hart->irq_nesting_depth == 0) {
		hart->prev_irq_state = prev_irq_state;
	}
	hart->irq_nesting_depth += 1;
}

void hartlock_release(void) {
	if (get_irq_state() != 0) {
		panic("hartlock_release: interruptable");
	}
	struct hart_locals *hart = get_hart_locals();
	if (hart->irq_nesting_depth < 1) {
		panic("hartlock_release: called without matching acquire");
	}
	hart->irq_nesting_depth -= 1;
	if (hart->irq_nesting_depth == 0 && hart->prev_irq_state) {
		irq_enable();
	}
}

