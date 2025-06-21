#include <panic.h>
#include <print.h>

/**
 * Performs architecture-specific operations to ensure the system stops if we
 * get an exception while printing the panic message.
 */
void _panic_begin(void);

/**
 * Halts the system.
 */
[[noreturn]]
void _panic_halt(void);

/**
 * Starts collecting a backtrace. Returns zero at the end of the backtrace.
 */
uptr _backtrace_begin(void);

/**
 * Moves from one frame to the next frame.
 */
uptr _backtrace_next(uptr);

/**
 * Returns the program counter at a backtrace frame.
 */
uaddr _backtrace_pc(uptr);

[[noreturn]]
void __panic(const char *file, usize line, const char *func, const char *msg,
             va_list ap) {

  _panic_begin();
  print("Kernel panic: {va}", msg, ap);
  print("  in {cstr} at {cstr}:{usize}", func, file, line);

  uptr backtrace = _backtrace_begin();
  uaddr pc;
  while ((pc = _backtrace_pc(backtrace))) {
    print("{uaddr}", pc);
    backtrace = _backtrace_next(backtrace);
  }
  _panic_halt();
}
