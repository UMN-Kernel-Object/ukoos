#include <print.h>
#include <types.h>

uptr _backtrace_begin(void) {
  register uptr ra __asm__("ra");
  print("{uptr}", ra);
  return 0;
}

uptr _backtrace_next(uptr) { return 0; }

uaddr _backtrace_pc(uptr) { return 0; }
