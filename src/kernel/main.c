/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <devicetree.h>
#include <hart_locals.h>
#include <init.h>
#include <mm/alloc.h>
#include <mm/physical_alloc.h>
#include <mm/virtual_alloc.h>
#include <panic.h>
#include <print.h>
#include <random.h>
#include <selftest.h>
#include <symbolicate.h>

[[noreturn]]
void main(u64 hart_id, paddr devicetree_start, paddr kernel_start,
          paddr kernel_end, const struct Elf64_Sym *symtab, usize symtab_len,
          const char *strtab, usize strtab_len) {
  print("Starting to boot ukoOS...");
  symbolicate_init(symtab, symtab_len, strtab, strtab_len);
  entropy_pool_init();
  arch_entropy_pool_seed_early();
  init_boothart_hart_locals(hart_id);
  mm_alloc_init();

  // Parse the Devicetree.
  struct devicetree_node *devicetree = devicetree_parse_from_physical(
      devicetree_start, kernel_start, kernel_end);
  if (!devicetree)
    panic("Failed to parse Devicetree");
  devicetree_add_entropy(devicetree);

  // Initialize the allocator.
  mm_init_physical(devicetree);
  if (!vma_allocator_init(&kernel_virtual_allocator, 0xffffffe000000000,
                          0xffffffffc0000000))
    panic("Failed to initialize kernel VMA allocator");
  assert(vma_alloc_by_addr(&kernel_virtual_allocator, 0xffffffe000000000,
                           0xffffffe000400000));
  assert(vma_alloc_by_addr(&kernel_virtual_allocator, 0xffffffe000400000,
                           0xffffffe000600000));
  vma_allocator_print(&kernel_virtual_allocator);

  // Run initializers, which include e.g. registering drivers.
  run_initializers();

  print("Running self-tests...");
  run_selftests();

  TODO();
}
