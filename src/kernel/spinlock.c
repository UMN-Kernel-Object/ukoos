#include "hartlock.h"
#include "panic.h"
#include "spinlock.h"

/* Returns 1 if thread is holding the specified spinlock */
u32 holding(struct spinlock *sp) {
	struct hart_locals *hart = get_hart_locals();
	return ((sp->state == 1) && (sp->hart == hart));
}

void initlock(struct spinlock *sp, const char *name) {
	sp->name = name;
	sp->state = 0;
	sp->hart = 0;
}

void acquire(struct spinlock *sp) {
	hart_lock();  // Lock execuation to current thread
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
	hart_unlock();  // Unlock execuation if necessary
}

