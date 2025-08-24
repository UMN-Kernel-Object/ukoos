#include "panic.h"
#include "print.h"
#include "spinlock.h"

int holding(struct spinlock *sp) {
	struct hart_locals *hart = get_hart_locals();
	return ((sp->state == 1) && (sp->hart == hart));
}

void pushoff() {
	int old_state = intr_get();
	intr_off();
	struct hart_locals *hart = get_hart_locals();
	if (hart->noff == 0) {
		hart->intena = old_state;
	}
	hart->noff += 1;
}

void popoff() {
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

void initlock(struct spinlock *sp, const char *name) {
	sp->name = name;
	sp->state = 0;
	sp->hart = 0;
}

void acquire(struct spinlock *sp) {
	pushoff();  // ensure interrupts disabled
	if (holding(sp)) {
		panic("acquire: {name}", sp->name);
	}
	while (__sync_lock_test_and_set(&sp->state, 1) != 0) {}
	__sync_synchronize();
	sp->hart = get_hart_locals();
}

void release(struct spinlock *sp) {
	if (!holding(sp)) {
		panic("release: {name}", sp->name);
	}
	sp->hart = 0;
	__sync_synchronize();
	__sync_lock_release(&sp->state);  // sp->state = 0;
	popoff();  // enable interrupts if needed
}

