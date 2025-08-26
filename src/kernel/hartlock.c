#include "hart_locals.h"
#include "hartlock.h"
#include "panic.h"

void hart_lock() {
	int old_state = intr_get();
	intr_off();
	struct hart_locals *hart = get_hart_locals();
	if (hart->noff == 0) {
		hart->intena = old_state;
	}
	hart->noff += 1;
}

void hart_unlock() {
	if (intr_get()) {
		panic("popoff: interruptable");
	}
	struct hart_locals *hart = get_hart_locals();
	if (hart->noff < 1) {
		panic("popoff: noff < 1");
	}
	hart->noff -= 1;
	if (hart->noff == 0 && hart->intena) {
		intr_on();
	}
}

