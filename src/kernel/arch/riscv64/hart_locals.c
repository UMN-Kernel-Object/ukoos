/*
 * SPDX-FileCopyrightText: 2025-2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <arch/riscv64/insns.h>
#include <device.h>
#include <hart_locals.h>
#include <mm/alloc.h>
#include <task.h>

/**
 * Storage for the boothart's hart-locals.
 */
static struct hart_locals boothart_hart_locals;

/**
 * The boothart's hart ID.
 */
static u64 boothart_hart_id;

/**
 * Modifies a heap to change the hart that owns it. This is only needed when
 * initializing the boothart, so it's declared here instead of in a public
 * header.
 */
void heap_change_boothart_hart(struct mm_alloc_heap *heap, struct hart *hart);

struct hart_locals *get_hart_locals(struct hartlock *_hartlock) {
  return (struct hart_locals *)csrr(RISCV64_CSR_SSCRATCH);
}

void init_boothart_hart_locals_early(u64 hart_id) {
  boothart_hart_id = hart_id;
  csrw(RISCV64_CSR_SSCRATCH, (u64)&boothart_hart_locals);
  boothart_hart_locals = (struct hart_locals){
      .hart = nullptr,
      .task = nullptr,
      .heap = nullptr,
      .rng = {},
  };
}

void init_boothart_hart_locals_late(void) {
  // Reallocate the hart-locals to be heap-allocated.
  struct hart_locals *hart_locals = alloc(sizeof(struct hart_locals));
  memcpy(hart_locals, &boothart_hart_locals, sizeof(struct hart_locals));
  csrw(RISCV64_CSR_SSCRATCH, (u64)hart_locals);

  // Find the hart object with the current hart ID, and ensure that it has a
  // hart group.
  for (struct list_head *hart_list = harts.next; hart_list != &harts;
       hart_list = hart_list->next) {
    struct hart *hart = container_of(hart_list, struct hart, list);
    if (hart->id == boothart_hart_id) {
      hart_locals->hart = hart;
      break;
    }
  }
  if (!hart_locals->hart) {
    print_devices();
    panic("Could not find a hart with the ID {u64}", boothart_hart_id);
  }

  // Fix up the heap to refer to this hart.
  heap_change_boothart_hart(hart_locals->heap, hart_locals->hart);

  // Create a task object for the current task.
  hart_locals->task = alloc(sizeof(struct task));
}

void init_hart_locals(u64 hart_id);
