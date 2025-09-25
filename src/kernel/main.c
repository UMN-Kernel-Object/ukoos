/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <devicetree.h>
#include <hart_locals.h>
#include <mm/alloc.h>
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

  print("Running self-tests...");
  run_selftests();

  struct devicetree_node *devicetree = devicetree_parse_from_physical(
      devicetree_start, kernel_start, kernel_end);
  if (!devicetree)
    panic("Failed to parse Devicetree");
  devicetree_add_entropy(devicetree);
  devicetree_print(devicetree);
  devicetree_free(devicetree);

  uptr a = mm_va_alloc(mm_kernel_virtual_buddy, 2 * 1024 * 1024);
  uptr b = mm_va_alloc(mm_kernel_virtual_buddy, 4096);
  print("{uptr} {uptr}", a, b);

  TODO();
}
