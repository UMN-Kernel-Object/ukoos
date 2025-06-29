#include <devicetree.h>
#include <mm/virtual_alloc.h>
#include <panic.h>
#include <print.h>
#include <symbolicate.h>

[[noreturn]]
void main(u64 hart_id, paddr devicetree_start, paddr kernel_start,
          paddr kernel_end, const struct Elf64_Sym *symtab, usize symtab_len,
          const char *strtab, usize strtab_len, uptr free_va_start,
          uptr free_va_end) {
  print("Starting to boot ukoOS...");
  symbolicate_init(symtab, symtab_len, strtab, strtab_len);
  devicetree_init(devicetree_start);
  devicetree_mm_init(kernel_start, kernel_end, &free_va_start, &free_va_end);
  mm_init_virtual(free_va_start, free_va_end);

  uptr a = mm_va_alloc(mm_kernel_virtual_buddy, 2 * 1024 * 1024);
  uptr b = mm_va_alloc(mm_kernel_virtual_buddy, 4096);
  print("{uptr} {uptr}", a, b);

  TODO();
}
