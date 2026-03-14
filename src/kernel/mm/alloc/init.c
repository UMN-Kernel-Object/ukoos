/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "heap.h"
#include <hart_locals.h>

/**
 * The heap used by the boothart.
 */
static struct mm_alloc_heap boothart_heap;

/**
 * Whether mm_alloc_init has been called.
 */
static bool inited = false;

// TODO: Bogus provenance, but this is the only easy way to not have issues
// with relocation sizes...
static struct mm_alloc_segment *boothart_heap_segment =
    (struct mm_alloc_segment *)0xffffffe000000000;

void mm_alloc_init(void) {
  // Make sure this function is only called once.
  assert(!inited);
  inited = true;

  // interrupts are disabled anyways, but this makes calling get_hart_locals
  // easier
  WITH_HARTLOCK(hartlock);

  // Initialize the heap.
  heap_init(hartlock, &boothart_heap, boothart_heap_segment);

  // Store the heap into the hart locals.
  struct hart_locals *hart_locals = get_hart_locals(hartlock);
  assert(!hart_locals->heap);
  hart_locals->heap = &boothart_heap;
}
