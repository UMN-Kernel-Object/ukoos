#include "panic.h"
#include "print.h"
#include "spinlock.h"

int holding(struct spinlock *sp) {
	return ((sp->state == 1) && (sp->cpu == CPU));
}

void pushoff();

void pushon();

void initlock(struct spinlock *sp, const char *name) {
	sp->name = name;
	sp->state = 0;
	// sp->cpu = 0;
}

void acquire(struct spinlock *sp) {
	// disableInterrupts();
	if (holding(sp)) {
		panic("acquire: {name}", sp->name);
	}
	while (__sync_lock_test_and_set(&sp->state, 1) != 0) {}
	__sync_synchronize();
	// sp->cpu = ?;
}

void release(struct spinlock *sp) {
	if (!holding(sp)) {
		panic("release: {name}", sp->name);
	}
	__sync_synchronize();
	// sp->state = 0;
	__sync_lock_release(&sp->state);
	// enableInterrupts();
}

