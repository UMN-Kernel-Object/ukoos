#ifndef UKO_OS_KERNEL__SPINLOCK_H
#define UKO_OS_KERNEL__SPINLOCK_H 1

#include "hart_locals.h"
#include "types.h"

struct spinlock {
	struct hart_locals *hart;  // Hart holding the lock
	const char *name;          // Name of the lock
	u32 state;                 // Is the lock acquired?
};

void initlock(struct spinlock *sp, const char *name);

void acquire(struct spinlock *sp);

void release(struct spinlock *sp);

#endif // UKO_OS_KERNEL__SPINLOCK_H

