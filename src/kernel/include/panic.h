#ifndef UKO_OS_KERNEL__PANIC_H
#define UKO_OS_KERNEL__PANIC_H 1

#include <compiler.h>

void assert(bool);
noreturn void panic(void);
noreturn void todo(void);

#endif // UKO_OS_KERNEL__PANIC_H
