/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__SYMBOLICATE_H
#define UKO_OS_KERNEL__SYMBOLICATE_H 1

#include <types.h>

struct Elf64_Sym;

struct symbolicated {
  const char *name;
  usize offset;
};

void symbolicate_init(const struct Elf64_Sym *symtab, usize symtab_len,
                      const char *strtab, usize strtab_len);
bool symbolicate(struct symbolicated *out, uaddr addr);

#endif // UKO_OS_KERNEL__SYMBOLICATE_H
