#include "hart_locals.h"
#include "hartlock.h"
#include "panic.h"
#include "types.h"

void hart_lock(void) {
	u32 old_state = intr_get();
	intr_off();
	struct hart_locals *hart = get_hart_locals();
	if (hart->noff == 0) {
		hart->intena = old_state;
	}
	hart->noff += 1;
}

void hart_unlock(void) {
	if (intr_get()) {
		panic("hart_unlock: interruptable");
	}
	struct hart_locals *hart = get_hart_locals();
	if (hart->noff < 1) {
		panic("hart_unlock: noff < 1");
	}
	hart->noff -= 1;
	if (hart->noff == 0 && hart->intena) {
		intr_on();
	}
}

