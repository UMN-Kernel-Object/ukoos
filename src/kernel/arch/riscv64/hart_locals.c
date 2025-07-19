#include <hart_locals.h>

/**
 * Storage for the boothart's hart-locals.
 */
struct hart_locals boothart_hart_locals;

/**
 * The heap used by the boothart.
 */
static struct mm_alloc_heap boothart_heap;

/**
 * The segment used in the initial pages.
 */
static __attribute__((aligned(1 << MM_ALLOC_SEGMENT_SHIFT))) union {

  struct mm_alloc_segment segment;
  char bytes[1 << MM_ALLOC_SEGMENT_SHIFT];
} boothart_heap_segment;

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
      .heap = &boothart_heap,
      .rng = {},
  };
  mm_alloc_boothart_heap_init(&boothart_heap, &boothart_heap_segment.segment);
}

void init_hart_locals(u64 hart_id);
