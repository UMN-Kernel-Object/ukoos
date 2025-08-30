#ifndef UKO_OS_KERNEL__INIT_H
#define UKO_OS_KERNEL__INIT_H 1

#include <macro_utils.h>
#include <types.h>

struct initializer {
  void (*func)(void);
  const char *file;
  u32 line;
  i32 priority;
};

#define _DEFINE_INIT(PRIORITY, N)                                              \
  static void __PASTE(__init_func_, N)(void);                                  \
  [[gnu::section(".initializers,\"wa\",@progbits#"), gnu::used]]               \
  static struct initializer __PASTE(__initializer_, N)[1] = {{                 \
      __PASTE(__init_func_, N),                                                \
      __FILE__,                                                                \
      __LINE__,                                                                \
      (PRIORITY),                                                              \
  }};                                                                          \
  static void __PASTE(__init_func_, N)(void)

/**
 * Defines an initializer.
 */
#define DEFINE_INIT(PRIORITY) _DEFINE_INIT(PRIORITY, __COUNTER__)

/**
 * Runs all initializers.
 */
void run_initializers(void);

#endif // UKO_OS_KERNEL__INIT_H
