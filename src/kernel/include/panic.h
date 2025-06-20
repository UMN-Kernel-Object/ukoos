#ifndef UKO_OS_KERNEL__PANIC_H
#define UKO_OS_KERNEL__PANIC_H 1

#include <compiler.h>

noreturn void _assert_fail(void);
noreturn void panic(void);
noreturn void todo(void);

static inline void assert(bool cond) {
  if (!cond)
    _assert_fail();
}

#endif // UKO_OS_KERNEL__PANIC_H
