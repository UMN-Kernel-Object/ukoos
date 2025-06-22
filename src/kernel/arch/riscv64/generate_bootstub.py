#!/usr/bin/env python3

from argparse import ArgumentParser
from dataclasses import dataclass
from pathlib import Path
import shutil
import struct
from tempfile import NamedTemporaryFile
from typing import Iterator, Union


@dataclass
class Pte:
    ppn: int
    vaddr: int
    flags: int


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
            if self.level == 2 and i > 256:
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
                elif pgtbl.level == 0:
                    assert isinstance(child, Pte)
                    assert child_addr == child.vaddr
                else:
                    assert isinstance(child, Pgtbl)
                    assert child.level == pgtbl.level - 1
                    for addr_and_pgtbl in loop(child_addr, child):
                        yield addr_and_pgtbl

        return loop(0, self.pgtbl2)

    def map(self, paddr: int, vaddr: int, flags: int) -> Pte:
        assert (paddr & 0xFFF) == 0
        ppn = paddr >> 12

        assert (vaddr & 0xFFF) == 0
        vpn0 = (vaddr >> 12) & 0x1FF

        pgtbl0 = self.walk(vaddr)
        assert pgtbl0.level == 0
        assert pgtbl0.children[vpn0] == None
        pte = Pte(ppn, vaddr, flags)
        pgtbl0.children[vpn0] = pte
        return pte

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


def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument("kernel", type=Path)
    arg_parser.add_argument("bootstub", type=Path)
    args = arg_parser.parse_args()

    # Create PTEs for all the mappings we want to make, so we can measure the
    # size of the page tables. This is the kernel, the stack page, and the root
    # page table.
    pgtbls = Pgtbls()
    kernel_ptes: list[tuple[int, int, Pte]] = []
    for phdr in read_phdrs(args.kernel):
        if phdr.type == 1:  # PT_LOAD
            flags = 0xC1  # PTE.D | PTE.A | PTE.V
            if phdr.flags & (1 << 0):  # PF_X
                flags |= 0x08  # PTE.X
            if phdr.flags & (1 << 1):  # PF_W
                flags |= 0x04  # PTE.W
            if phdr.flags & (1 << 2):  # PF_R
                flags |= 0x02  # PTE.R
            for offset in range(0, phdr.memsz, 4096):
                kernel_ptes.append(
                    (
                        phdr.offset + offset,
                        max(0, min(4096, phdr.filesz - offset)),
                        pgtbls.map(0, phdr.vaddr + offset, flags),
                    )
                )
    stack_page_pte = pgtbls.map(
        0,
        0xFFFFFFFFFFFFC000,  # stack page
        0xC7,  # PTE.D | PTE.A | PTE.W | PTE.R | PTE.V
    )
    root_pgtbl_pte = pgtbls.map(
        0,
        0xFFFFFFFFFFFFD000,  # root page table
        0xC7,  # PTE.D | PTE.A | PTE.W | PTE.R | PTE.V
    )

    # Figure out the layout of everything in physical memory.
    next_paddr = 0x80080000  # start address
    next_paddr += 4096  # skip the bootstub
    pgtbls_start = next_paddr
    next_paddr += sum(4096 for _ in pgtbls.all_pgtbls())
    kernel_start = next_paddr
    next_paddr += 4096 * len(kernel_ptes)
    stack_start = next_paddr
    del next_paddr

    # Count how many trailing pages are purely zeroes.
    trailing_zero_pages = 0
    for _, file_len, _ in reversed(kernel_ptes):
        if file_len == 0:
            trailing_zero_pages += 1
        else:
            break

    # Start assigning addresses to kernel pages.
    kernel_section = f'.section .kernel, "a", @progbits\n'
    for i, (offset, file_len, pte) in enumerate(kernel_ptes):
        if i >= len(kernel_ptes) - trailing_zero_pages:
            assert file_len == 0
        if file_len > 0:
            kernel_section += f'.incbin "{args.kernel}", {offset}, {file_len}\n'
        if i == len(kernel_ptes) - trailing_zero_pages - 1:
            kernel_section += f'.section .kernel_bss, "a", @nobits\n'
        if file_len < 4096:
            kernel_section += f".zero {4096 - file_len}\n"
        pte.ppn = (kernel_start + 4096 * i) >> 12

    # Assign addresses to the other pages.
    stack_page_pte.ppn = stack_start >> 12
    root_pgtbl_pte.ppn = pgtbls_start >> 12

    # Compute the addresses of the page tables.
    pgtbl_paddrs = {}
    for addr, pgtbl in pgtbls.all_pgtbls():
        name = f"boot_pgtbl_{pgtbl.level}_{addr:016x}"
        pgtbl_paddrs[name] = pgtbls_start + 4096 * len(pgtbl_paddrs)

    # Write to a temporary file, so that a crash partway through doesn't
    # confuse make into thinking we successfully completed next run.
    with NamedTemporaryFile("w") as tmp:
        # Write the address of the entrypoint.
        print(".section .data", file=tmp)
        print(".global entrypoint", file=tmp)
        print(".p2align 3", file=tmp)
        print(".type entrypoint, @object", file=tmp)
        print(".size entrypoint, 8", file=tmp)
        print(f"entrypoint: .8byte {hex(read_entrypoint(args.kernel))}\n", file=tmp)

        # Write the page tables.
        print('.section .pgtbls, "a", @progbits', file=tmp)
        print(f".global boot_pgtbl_2_0000000000000000", file=tmp)
        for addr, pgtbl in pgtbls.all_pgtbls():
            name = f"boot_pgtbl_{pgtbl.level}_{addr:016x}"
            print(".p2align 12", file=tmp)
            print(f".type {name}, @object", file=tmp)
            print(f".size {name}, 4096", file=tmp)
            print(f"{name}: # should be at {pgtbl_paddrs[name]:#x}", file=tmp)
            entries = []
            for child_addr, child in pgtbl.children_addrs(addr):
                if child == None:
                    entry = "0"
                elif pgtbl.level == 0:
                    assert isinstance(child, Pte)
                    assert child_addr == child.vaddr
                    assert child.ppn != 0
                    entry = hex((child.ppn << 10) | child.flags)
                else:
                    assert isinstance(child, Pgtbl)
                    assert child.level == pgtbl.level - 1
                    child_name = f"boot_pgtbl_{child.level}_{child_addr:016x}"
                    entry = hex((pgtbl_paddrs[child_name] >> 2) | 0x01)
                entries.append(entry)
            print(f"    .8byte {', '.join(entries)}", file=tmp)

        # Write the kernel section.
        print(f'.section .kernel, "a", @progbits\n{kernel_section}', file=tmp)

        # Copy the temporary file to the output file.
        tmp.flush()
        shutil.copy(tmp.name, args.bootstub)


if __name__ == "__main__":
    main()
