#include <hart_locals.h>

/**
 * Storage for the boothart's hart-locals.
 */
struct hart_locals boothart_hart_locals;

struct hart_locals *get_hart_locals(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
  register struct hart_locals *tp __asm__("tp");
  return tp;
#pragma GCC diagnostic pop
}

void init_boothart_hart_locals(u64 hart_id) {
  __asm__ volatile("la tp, %0\n" : : "i"(&boothart_hart_locals) :);
  boothart_hart_locals = (struct hart_locals){
      .hart_id = hart_id,
      .rng = {},
  };
}

void init_hart_locals(u64 hart_id);
