#include <panic.h>

noreturn void _panic_halt(void);

noreturn void _assert_fail(void) { panic(); }

noreturn void panic(void) { _panic_halt(); }

noreturn void todo(void) { panic(); }
