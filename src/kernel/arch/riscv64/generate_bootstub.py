#!/usr/bin/env python3

# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from argparse import ArgumentParser
from contextlib import ExitStack
from dataclasses import dataclass
import logging
from pathlib import Path
import shutil
import struct
from tempfile import NamedTemporaryFile
from typing import Generator, Iterator, Optional, Union


logger = logging.getLogger(__file__)


def flags_to_str(flags: int) -> str:
    assert flags < 256
    return "".join(
        (ch if (flags & (1 << i)) else "-")
        for i, ch in reversed(list(enumerate("VRWXUGAD")))
    )


@dataclass
class Pte:
    ppn: Optional[int]
    vaddr: int
    flags: int

    def __str__(self):
        if self.ppn is not None:
            paddr = f"{self.ppn << 12:#x}"
        else:
            paddr = "None"
        flags = flags_to_str(self.flags)
        return f"Pte(paddr={paddr}, vaddr={self.vaddr:#x}, flags={flags!r})"


@dataclass
class Pgtbl:
    children: list[Union["Pgtbl", Pte, None]]
    level: int
    mapped: bool

    def __init__(self, *, level: int):
        self.children = [None for _ in range(512)]
        self.level = level
        self.mapped = False

    def children_addrs(
        self, start_addr: int
    ) -> Iterator[tuple[int, Union["Pgtbl", Pte, None]]]:
        for i, child in enumerate(self.children):
            child_addr = start_addr + (i << (12 + 9 * self.level))
            if self.level == 2 and i >= 256:
                child_addr |= 0xFFFFFFC000000000
            yield (child_addr, child)


@dataclass
class Pgtbls:
    pgtbl2: Pgtbl

    def __init__(self):
        self.pgtbl2 = Pgtbl(level=2)

    def all_pgtbls(self) -> Iterator[tuple[int, Pgtbl]]:
        def loop(start_addr: int, pgtbl: Pgtbl) -> Iterator[tuple[int, Pgtbl]]:
            yield (start_addr, pgtbl)
            for child_addr, child in pgtbl.children_addrs(start_addr):
                if child == None:
                    continue
                elif isinstance(child, Pte):
                    assert child_addr == child.vaddr
                else:
                    assert isinstance(child, Pgtbl)
                    assert child.level == pgtbl.level - 1
                    for addr_and_pgtbl in loop(child_addr, child):
                        yield addr_and_pgtbl

        return loop(0, self.pgtbl2)

    def _log_map(self, vaddr: int, flags: int, memsz: int, label: str):
        logger.info(
            f"[{flags_to_str(flags)}] {vaddr:#018x}-{vaddr+memsz-1:#018x}: {label} ({memsz // 1024} K)"
        )

    def map(
        self,
        paddr: Optional[int],
        vaddr: int,
        flags: int,
        memsz: int,
        label: str,
    ) -> Generator[tuple[int, Pte], None, None]:
        self._log_map(vaddr, flags, memsz, label)
        for i in range(0, memsz, 4096):
            yield (
                i,
                self.map_one(
                    paddr + i if paddr is not None else None, vaddr + i, flags, None
                ),
            )

    def map_now(
        self,
        paddr: int,
        vaddr: int,
        flags: int,
        memsz: int,
        label: str,
    ) -> int:
        for _ in self.map(paddr, vaddr, flags, memsz, label):
            pass
        return paddr

    def map_one(
        self, paddr: Optional[int], vaddr: int, flags: int, label: Optional[str]
    ) -> Pte:
        if label is not None:
            self._log_map(vaddr, flags, 4096, label)

        if paddr is not None:
            assert (paddr & 0xFFF) == 0
            ppn = paddr >> 12
        else:
            ppn = None

        assert (vaddr & 0xFFF) == 0
        vpn0 = (vaddr >> 12) & 0x1FF

        pgtbl0 = self.walk(vaddr)
        assert pgtbl0.level == 0
        assert pgtbl0.children[vpn0] == None
        pte = Pte(ppn, vaddr, flags)
        pgtbl0.children[vpn0] = pte
        return pte

    def map_1G(
        self, paddr: Optional[int], vaddr: int, flags: int, memsz: int, label: str
    ):
        assert (memsz & 0x3FFFFFFF) == 0
        self._log_map(vaddr, flags, memsz, label)
        for i in range(0, memsz, 1 << 30):
            self.map_one_1G(paddr + i if paddr is not None else None, vaddr + i, flags)

    def map_one_1G(self, paddr: Optional[int], vaddr: int, flags: int):
        if paddr is not None:
            assert (paddr & 0x3FFFFFFF) == 0
            ppn = paddr >> 12
        else:
            ppn = None

        assert (vaddr & 0x3FFFFFFF) == 0
        vpn2 = (vaddr >> 30) & 0x1FF
        expected_high_bits = 0 if vpn2 < 256 else 0x1FFFFFF
        assert (vaddr >> 39) == expected_high_bits

        self.pgtbl2.children[vpn2] = Pte(ppn, vaddr, flags)

    def walk(self, vaddr: int) -> Pgtbl:
        assert (vaddr & 0xFFF) == 0
        vpn1 = (vaddr >> 21) & 0x1FF
        vpn2 = (vaddr >> 30) & 0x1FF

        if self.pgtbl2.children[vpn2] == None:
            self.pgtbl2.children[vpn2] = Pgtbl(level=1)
        pgtbl1 = self.pgtbl2.children[vpn2]
        assert isinstance(pgtbl1, Pgtbl)

        if pgtbl1.children[vpn1] == None:
            pgtbl1.children[vpn1] = Pgtbl(level=0)
        pgtbl0 = pgtbl1.children[vpn1]
        assert isinstance(pgtbl0, Pgtbl)

        return pgtbl0


@dataclass
class ProgramHeader:
    type: int
    flags: int
    offset: int
    vaddr: int
    paddr: int
    filesz: int
    memsz: int
    align: int


@dataclass
class SectionHeader:
    name: int
    type: int
    flags: int
    addr: int
    offset: int
    size: int
    link: int
    info: int
    addralign: int
    entsize: int


def read_entrypoint(elf_path: Path) -> int:
    with elf_path.open("rb") as f:
        f.seek(24)
        (e_entry,) = struct.unpack("<Q", f.read(8))
        return e_entry


def read_phdrs(elf_path: Path) -> Iterator[ProgramHeader]:
    with elf_path.open("rb") as f:
        f.seek(32)
        (e_phoff,) = struct.unpack("<Q", f.read(8))
        f.seek(56)
        (e_phnum,) = struct.unpack("<H", f.read(2))

        f.seek(e_phoff)
        for _ in range(e_phnum):
            yield ProgramHeader(*struct.unpack("<LLQQQQQQ", f.read(56)))


def read_shdrs(elf_path: Path) -> Iterator[SectionHeader]:
    with elf_path.open("rb") as f:
        f.seek(40)
        (e_shoff,) = struct.unpack("<Q", f.read(8))
        f.seek(60)
        (e_shnum,) = struct.unpack("<H", f.read(2))

        f.seek(e_shoff)
        for _ in range(e_shnum):
            yield SectionHeader(*struct.unpack("<LLQQQQLLQQ", f.read(64)))


def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument("-v", "--verbose", action="store_true")
    arg_parser.add_argument("kernel", type=Path)
    arg_parser.add_argument("linker_script", type=Path)
    arg_parser.add_argument("bootstub", type=Path)
    arg_parser.add_argument("bootstub_startaddr", type=lambda x: int(x,0))
    args = arg_parser.parse_args()

    # Configure the logger.
    logging.basicConfig(
        format="{message}",
        level=logging.INFO if args.verbose else logging.WARNING,
        style="{",
    )

    # Create the page tables.
    pgtbls = Pgtbls()

    # Create the physical memory identity map.
    pgtbls.map_1G(
        0,
        0xFFFFFFC000000000,
        0xE7,  # PTE.D | PTE.A | PTE.G | PTE.W | PTE.R | PTE.V
        128 << 30,
        "physical map",
    )

    # Find the kernel's symbol table and string table.
    shdrs = list(read_shdrs(args.kernel))
    symtab = None
    for shdr in shdrs:
        if shdr.type == 2:  # SHT_SYMTAB
            if symtab != None:
                raise Exception("Kernel had multiple SHT_SYMTAB sections")
            symtab = shdr
    if symtab == None:
        raise Exception("Kernel had no SHT_SYMTAB sections")
    strtab = shdrs[symtab.link]
    if strtab.type != 3:  # SHT_STRTAB
        raise Exception(
            "Symbol table's link field didn't refer to a SHT_STRTAB section"
        )

    # Load all the PT_LOAD program headers.
    phdrs = [phdr for phdr in read_phdrs(args.kernel) if phdr.type == 1]

    # Create PTEs for all the mappings we want to make, so we can measure the
    # size of the page tables. This is the kernel, the stack page, and the root
    # page table.
    kernel_ptes: list[tuple[int, int, Pte]] = []
    kernel_tables_ptes: list[Pte] = []
    max_kernel_va = 0
    for i, phdr in enumerate(phdrs):
        flags = 0xE1  # PTE.D | PTE.A | PTE.G | PTE.V
        if phdr.flags & (1 << 0):  # PF_X
            flags |= 0x08  # PTE.X
        if phdr.flags & (1 << 1):  # PF_W
            flags |= 0x04  # PTE.W
        if phdr.flags & (1 << 2):  # PF_R
            flags |= 0x02  # PTE.R
        kernel_ptes.extend(
            (phdr.offset + i, max(0, min(4096, phdr.filesz - i)), pte)
            for i, pte in pgtbls.map(
                None, phdr.vaddr, flags, phdr.memsz, f"kernel phdr {i}"
            )
        )
        max_kernel_va = max(max_kernel_va, (phdr.vaddr + phdr.memsz + 4095) & ~4095)
    if max_kernel_va == 0:
        raise Exception("Kernel mapped no memory")
    symtab_va = max_kernel_va
    kernel_tables_ptes.extend(
        pte
        for _, pte in pgtbls.map(
            None,
            symtab_va,
            0xE3,  # PTE.D | PTE.A | PTE.G | PTE.R | PTE.V
            symtab.size + strtab.size,
            "symbol and string tables",
        )
    )

    # Figure out the layout of the bootstub, the kernel, and the symbol and
    # string tables in physical memory.
    physical_memory_layout = []

    def reserve(label, length):
        nonlocal next_paddr
        nonlocal physical_memory_layout
        old_paddr = next_paddr
        next_paddr += length
        next_paddr = (next_paddr + 4095) & ~4095
        size_kib = (next_paddr - old_paddr) // 1024
        physical_memory_layout.append((old_paddr, next_paddr, size_kib, label))
        return old_paddr

    total_start_paddr = next_paddr = args.bootstub_startaddr # start address
    reserve("bootstub", 4096)
    kernel_tables_start_paddr = reserve(
        "symbol and string tables", symtab.size + strtab.size
    )
    kernel_start_paddr = reserve("kernel", 4096 * len(kernel_ptes))

    # Allocate and map the initial heap segment and the boothart's stack.
    initial_heap_segment_start_paddr = pgtbls.map_now(
        reserve("initial heap segment", 4 << 20),
        0xFFFFFFE000000000,
        0xE7,  # PTE.D | PTE.A | PTE.G | PTE.W | PTE.R | PTE.V
        4 << 20,
        "initial heap segment",
    )
    boothart_stack_start_paddr = pgtbls.map_now(
        reserve("boothart stack", 2 << 20),
        0xFFFFFFE000400000,
        0xE7,  # PTE.D | PTE.A | PTE.G | PTE.W | PTE.R | PTE.V
        2 << 20,
        "boothart stack",
    )

    # Reserve memory for the page tables.
    pgtbls_start_paddr = reserve("page tables", sum(4096 for _ in pgtbls.all_pgtbls()))

    # Print the physical memory layout.
    for line in (
        "┌──────────────────────────────────────────────────────────────────────────────┐",
        "│ Physical memory layout                                                       │",
        "├────────────────────┬────────────────────┬─────────┬──────────────────────────┤",
        "│ Start Address      │ End Address        │ Size    │ Label                    │",
        "├────────────────────┼────────────────────┼─────────┼──────────────────────────┤",
    ):
        logger.info(line)
    for old_paddr, next_paddr, size_kib, label in physical_memory_layout:
        logger.info(
            f"│ {old_paddr:#018x} │ {next_paddr - 1:#018x} │ {size_kib:>5} K │ {label:<24} │"
        )
    for line in (
        "├────────────────────┼────────────────────┼─────────┼──────────────────────────┤",
        f"│ {total_start_paddr:#018x} │ {next_paddr - 1:#018x} │ {(next_paddr - total_start_paddr) // 1024:>5} K │ Total                    │",
        "└────────────────────┴────────────────────┴─────────┴──────────────────────────┘",
    ):
        logger.info(line)
    del next_paddr

    # Assign addresses to kernel symbol and string table pages.
    for i, pte in enumerate(kernel_tables_ptes):
        pte.ppn = (kernel_tables_start_paddr + 4096 * i) >> 12

    # Assign addresses to kernel pages.
    kernel_section_includes = []
    for i, (offset, file_len, pte) in enumerate(kernel_ptes):
        if file_len > 0:
            kernel_section_includes.append((offset, file_len))
        if file_len < 4096:
            kernel_section_includes.append((None, 4096 - file_len))
        pte.ppn = (kernel_start_paddr + 4096 * i) >> 12

    # Compute the addresses of the page tables.
    pgtbl_paddrs = {}
    for addr, pgtbl in pgtbls.all_pgtbls():
        name = f"boot_pgtbl_{pgtbl.level}_{addr:016x}"
        pgtbl_paddrs[name] = pgtbls_start_paddr + 4096 * len(pgtbl_paddrs)

    # Write to temporary files, so that a crash partway through doesn't confuse
    # make into thinking we successfully completed next run.
    with ExitStack() as exit_stack:
        tmp_bootstub = exit_stack.enter_context(NamedTemporaryFile("w"))
        tmp_linker = exit_stack.enter_context(NamedTemporaryFile("w"))

        # Write the addresses of the entrypoint and tables.
        print(".section .data", file=tmp_bootstub)
        data_constants = {
            "entrypoint": read_entrypoint(args.kernel),
            "initial_stack_va": 0xFFFFFFE0005FF000,
            "symtab_va": symtab_va,
            "symtab_len": symtab.size,
            "strtab_va": symtab_va + symtab.size,
            "strtab_len": strtab.size,
        }
        for name, value in data_constants.items():
            print(f".global {name}", file=tmp_bootstub)
            print(f".p2align 3", file=tmp_bootstub)
            print(f".type {name}, @object", file=tmp_bootstub)
            print(f".size {name}, 8", file=tmp_bootstub)
            print(f"{name}: .8byte {hex(value)}\n", file=tmp_bootstub)

        # Write the page tables.
        print('.section .pgtbls, "a", @progbits', file=tmp_bootstub)
        # print(f".global boot_pgtbl_2_0000000000000000", file=tmp)
        for addr, pgtbl in pgtbls.all_pgtbls():
            name = f"boot_pgtbl_{pgtbl.level}_{addr:016x}"
            print(".p2align 12", file=tmp_bootstub)
            print(f".global {name}", file=tmp_bootstub)
            print(f".type {name}, @object", file=tmp_bootstub)
            print(f".size {name}, 4096", file=tmp_bootstub)
            print(f"{name}:", file=tmp_bootstub)
            print(
                f'ASSERT({name} == {pgtbl_paddrs[name]:#x}, "{name} should have been at {pgtbl_paddrs[name]:#x}")',
                file=tmp_linker,
            )
            entries = []
            for child_addr, child in pgtbl.children_addrs(addr):
                if child == None:
                    entry = "0"
                elif isinstance(child, Pte):
                    assert child_addr == child.vaddr
                    assert child.ppn is not None, child
                    entry = hex((child.ppn << 10) | child.flags)
                else:
                    assert isinstance(child, Pgtbl)
                    assert child.level == pgtbl.level - 1
                    child_name = f"boot_pgtbl_{child.level}_{child_addr:016x}"
                    entry = hex((pgtbl_paddrs[child_name] >> 2) | 0x01)
                entries.append(entry)
            print(f"    .8byte {', '.join(entries)}", file=tmp_bootstub)

        # Write the kernel tables section.
        print(
            f'ASSERT(__start_kernel_tables == {kernel_tables_start_paddr:#x}, "symbol and string tables should have been at {kernel_tables_start_paddr:#x}")',
            file=tmp_linker,
        )
        print(f'.section .kernel_tables, "a", @progbits', file=tmp_bootstub)
        print(f".p2align 3", file=tmp_bootstub)
        print(f".type symtab, @object", file=tmp_bootstub)
        print(f".size symtab, {symtab.size}", file=tmp_bootstub)
        print(
            f'symtab: .incbin "{args.kernel}", {symtab.offset}, {symtab.size}',
            file=tmp_bootstub,
        )
        print(f".type strtab, @object", file=tmp_bootstub)
        print(f".size strtab, {strtab.size}", file=tmp_bootstub)
        print(
            f'strtab: .incbin "{args.kernel}", {strtab.offset}, {strtab.size}',
            file=tmp_bootstub,
        )

        # Write the kernel section.
        print(f'.section .kernel, "a", @progbits', file=tmp_bootstub)
        print(
            f'ASSERT(__start_kernel == {kernel_start_paddr:#x}, "kernel should have been at {kernel_start_paddr:#x}")',
            file=tmp_linker,
        )
        i = 0
        while i < len(kernel_section_includes):
            chunk_offset, chunk_len = kernel_section_includes[i]
            i += 1

            while i < len(kernel_section_includes) and kernel_section_includes[i][
                0
            ] == ((chunk_offset + chunk_len) if chunk_offset is not None else None):
                chunk_len += kernel_section_includes[i][1]
                i += 1

            if chunk_offset is not None:
                print(
                    f'.incbin "{args.kernel}", {chunk_offset}, {chunk_len}',
                    file=tmp_bootstub,
                )
            else:
                if all(
                    chunk_offset is None
                    for chunk_offset, _ in kernel_section_includes[i:]
                ):
                    print(f'.section .kernel_bss, "a", @nobits', file=tmp_bootstub)
                print(f".zero {chunk_len}", file=tmp_bootstub)

        # Output assertions about the location of the initial heap segment and the boothart's stack.
        print(
            f'ASSERT(__start_initial_heap_segment == {initial_heap_segment_start_paddr:#x}, "initial heap segment should have been at {initial_heap_segment_start_paddr:#x}")',
            file=tmp_linker,
        )
        print(
            f'ASSERT(__start_boothart_stack == {boothart_stack_start_paddr:#x}, "boothart stack should have been at {boothart_stack_start_paddr:#x}")',
            file=tmp_linker,
        )

        # Copy the temporary files to the output files.
        tmp_bootstub.flush()
        tmp_linker.flush()
        shutil.copy(tmp_bootstub.name, args.bootstub)
        shutil.copy(tmp_linker.name, args.linker_script)


if __name__ == "__main__":
    main()
