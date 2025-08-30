#include "hart_locals.h"
#include "hartlock.h"
#include "irq.h"
#include "panic.h"
#include "types.h"

void hart_lock(void) {
	u32 old_irq_state = get_irq_state();
	irq_disable();
	struct hart_locals *hart = get_hart_locals();
	if (hart->noff == 0) {
		hart->intena = old_irq_state;
	}
	hart->noff += 1;
}

void hart_unlock(void) {
	if (irq_get_state() != 0) {
		panic("hart_unlock: interruptable");
	}
	struct hart_locals *hart = get_hart_locals();
	if (hart->noff < 1) {
		panic("hart_unlock: noff < 1");
	}
	hart->noff -= 1;
	if (hart->noff == 0 && hart->intena) {
		irq_enable();
	}
}

