#ifndef UKO_OS_KERNEL__PANIC_H
#define UKO_OS_KERNEL__PANIC_H 1

#include <types.h>

[[noreturn]]
void __panic(const char *file, usize line, const char *func, const char *msg,
             va_list ap);

[[noreturn]]
static inline void _panic(const char *file, usize line, const char *func,
                          const char *msg, ...) {
  va_list ap;
  va_start(ap);
  __panic(file, line, func, msg, ap);
}

constexpr bool ASSERTIONS_ENABLED = true;

static inline void _assert(const char *file, usize line, const char *func,
                           bool cond, const char *cond_str, const char *msg,
                           ...) {
  va_list ap;
  va_start(ap);
  if (!cond)
    _panic(file, line, func, "{va}: {cstr}", msg, ap, cond_str);
  va_end(ap);
}

#define assert(cond, ...)                                                      \
  _assert(__FILE__, __LINE__, __func__, cond, #cond,                           \
          "assertion failed" __VA_OPT__(": ") __VA_ARGS__)
#define panic(...) _panic(__FILE__, __LINE__, __func__, "" __VA_ARGS__)
#define TODO(...)                                                              \
  _panic(__FILE__, __LINE__, __func__, "TODO" __VA_OPT__(": ") __VA_ARGS__)

#endif // UKO_OS_KERNEL__PANIC_H
