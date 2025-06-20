#include <panic.h>

noreturn void _panic_halt(void);

void assert(bool cond) {
  if (!cond)
    panic();
}

noreturn void panic(void) { _panic_halt(); }

noreturn void todo(void) { panic(); }
