#include <panic.h>
#include <print.h>
#include <symbolicate.h>

struct Elf64_Sym {
  u32 st_name;
  u8 st_info;
  u8 st_other;
  u16 st_sndx;
  u64 st_value;
};

enum Elf64_Sym_Type : u8 {
  STT_NOTYPE,
  STT_OBJECT,
  STT_FUNC,
  STT_SECTION,
  STT_FILE,
  STT_LOPROC,
  STT_HIPROC,
};

enum Elf64_Sym_Bind : u8 {
  STB_LOCAL,
  STB_GLOBAL,
  STB_WEAK,
  STB_LOPROC,
  STB_HIPROC,
};

static inline enum Elf64_Sym_Type st_type(u8 st_info) { return st_info & 0xf; }

static const struct Elf64_Sym *symtab_start = nullptr, *symtab_end = nullptr;
static const char *strtab = nullptr;
static usize strtab_len = 0;

void symbolicate_init(const struct Elf64_Sym *symtab, usize symtab_len,
                      const char *strtab_, usize strtab_len_) {
  assert(symtab);
  assert((symtab_len % sizeof(struct Elf64_Sym)) == 0);
  assert(strtab_);

  symtab_start = symtab;
  symtab_end = symtab + (symtab_len / sizeof(struct Elf64_Sym));
  strtab = strtab_;
  strtab_len = strtab_len_;
}

bool symbolicate(struct symbolicated *out, uaddr addr) {
  if (!symtab_start || !symtab_end) {
    return false;
  }

  bool found_match = false;
  const struct Elf64_Sym *best_match = nullptr;
  for (const struct Elf64_Sym *sym = symtab_start; sym != symtab_end; sym++) {
    if (sym->st_name == 0)
      continue;
    if (sym->st_name >= strtab_len)
      continue;
    if (st_type(sym->st_info) != STT_FUNC)
      continue;

    if (sym->st_value <= addr) {
      if (found_match) {
        if (best_match->st_value < sym->st_value)
          best_match = sym;
      } else {
        found_match = true;
        best_match = sym;
      }
    }
  }

  if (!found_match) {
    *out = (struct symbolicated){0};
    return false;
  } else {
    *out = (struct symbolicated){
        .name = &strtab[best_match->st_name],
        .offset = addr - best_match->st_value,
    };
    return true;
  }
}
