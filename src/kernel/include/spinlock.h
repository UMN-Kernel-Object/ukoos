#ifndef UKO_OS_KERNEL__SPINLOCK_H
#define UKO_OS_KERNEL__SPINLOCK_H 1

#include "hart_locals.h"
#include "types.h"

struct spinlock {
	u32 state;
	const char *name;
	struct hart_locals *hart;
};

void initlock(struct spinlock *sp, const char *name);

void acquire(struct spinlock *sp);

void release(struct spinlock *sp);

#endif // UKO_OS_KERNEL__SPINLOCK_H

