/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <devicetree.h>
#include <hart_locals.h>
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

  print("Running self-tests...");
  run_selftests();

  struct devicetree_node *devicetree = devicetree_parse_from_physical(
      devicetree_start, kernel_start, kernel_end);
  if (!devicetree)
    panic("Failed to parse Devicetree");
  devicetree_add_entropy(devicetree);
  mm_init_physical(devicetree);
  struct vma_allocator *kernel_virtual_allocator =
      vma_allocator_new(0xffffffe000000000, 0xffffffffc0000000);
  vma_allocator_print(kernel_virtual_allocator);
  assert(vma_alloc_by_addr(kernel_virtual_allocator, 0xffffffe000000000,
                           0xffffffe000400000));
  vma_allocator_print(kernel_virtual_allocator);
  assert(vma_alloc_by_addr(kernel_virtual_allocator, 0xffffffe000400000,
                           0xffffffe000600000));
  vma_allocator_print(kernel_virtual_allocator);

  struct vma *a = vma_alloc(kernel_virtual_allocator, 512);
  print("a = {uptr}", a);
  struct vma *b = vma_alloc(kernel_virtual_allocator, 512);
  print("b = {uptr}", b);
  struct vma *c = vma_alloc(kernel_virtual_allocator, 512);
  print("c = {uptr}", c);
  vma_allocator_print(kernel_virtual_allocator);
  vma_free(b);
  vma_free(c);
  vma_free(a);
  vma_allocator_print(kernel_virtual_allocator);

  TODO();
}
