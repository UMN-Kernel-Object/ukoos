#include <arch/riscv64/insns.h>
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
  return (struct hart_locals *)csrr(RISCV64_CSR_SSCRATCH);
}

void init_boothart_hart_locals(u64 hart_id) {
  csrw(RISCV64_CSR_SSCRATCH, (u64)&boothart_hart_locals);
  boothart_hart_locals = (struct hart_locals){
      .hart_id = hart_id,
      .heap = &boothart_heap,
      .rng = {},
  };
  mm_alloc_boothart_heap_init(&boothart_heap, &boothart_heap_segment.segment);
}

void init_hart_locals(u64 hart_id);
