#include <panic.h>
#include <print.h>
#include <symbolicate.h>

[[noreturn]]
void main(u64 hart_id, paddr devicetree_start, const struct Elf64_Sym *symtab,
          usize symtab_len, const char *strtab, usize strtab_len) {
  print("Starting to boot ukoOS...\n");
  symbolicate_init(symtab, symtab_len, strtab, strtab_len);

  TODO();
}
