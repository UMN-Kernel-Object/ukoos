#ifndef UKO_OS_KERNEL__PANIC_H
#define UKO_OS_KERNEL__PANIC_H 1

#include <types.h>

[[noreturn]]
void __panic(const char *file, usize line, const char *func, const char *msg,
             va_list ap);

static inline void _assert(const char *file, usize line, const char *func,
                           bool cond, const char *msg, ...) {
  va_list ap;
  va_start(ap);
  if (!cond)
    __panic(file, line, func, msg, ap);
}

[[noreturn]]
static inline void _panic(const char *file, usize line, const char *func,
                          const char *msg, ...) {
  va_list ap;
  va_start(ap);
  __panic(file, line, func, msg, ap);
}

#define assert(cond, ...)                                                      \
  _assert(__FILE__, __LINE__, __func__, cond,                                  \
          "assertion failed" __VA_OPT__(": ") __VA_ARGS__)
#define panic(...) _panic(__FILE__, __LINE__, __func__, "" __VA_ARGS__)
#define TODO(...)                                                              \
  _panic(__FILE__, __LINE__, __func__, "TODO" __VA_OPT__(": ") __VA_ARGS__)

#endif // UKO_OS_KERNEL__PANIC_H
