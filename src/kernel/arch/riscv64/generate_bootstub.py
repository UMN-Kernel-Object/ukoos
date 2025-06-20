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

    def map(self, paddr: int, vaddr: int, flags: int):
        assert (paddr & 0xFFF) == 0
        ppn = paddr >> 12

        assert (vaddr & 0xFFF) == 0
        vpn0 = (vaddr >> 12) & 0x1FF

        pgtbl0 = self.walk(vaddr)
        assert pgtbl0.level == 0
        assert pgtbl0.children[vpn0] == None
        pgtbl0.children[vpn0] = Pte(ppn, vaddr, flags)

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


def read_phdrs(elf_path: Path) -> Iterator[ProgramHeader]:
    with elf_path.open("rb") as f:
        f.seek(32, 0)
        (e_phoff,) = struct.unpack("<Q", f.read(8))
        f.seek(56, 0)
        (e_phnum,) = struct.unpack("<H", f.read(2))

        f.seek(e_phoff)
        for _ in range(e_phnum):
            yield ProgramHeader(*struct.unpack("<LLQQQQQQ", f.read(56)))


def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument("kernel", type=Path)
    arg_parser.add_argument("bootstub", type=Path)
    args = arg_parser.parse_args()

    # Compute the kernel start address in physical memory. We can do so,
    # because bootstub.ld ensures the bootstub is a single page.
    next_paddr = 0x80080000  # base address
    next_paddr += 4096  # length of .bootstub
    kernel_start = next_paddr

    # Map the kernel. Note that since we don't know how much memory we need up
    # front, we can't actually support .bss...
    #
    # If somebody wanted to add .bss support, they could probably compute how
    # many pages of .bss we need, put those at the end of the final ELF, and
    # use them as the backing pages.
    pgtbls = Pgtbls()
    for phdr in read_phdrs(args.kernel):
        if phdr.type == 1:  # PT_LOAD
            flags = 0xC1  # PTE.D | PTE.A | PTE.V
            if phdr.flags & (1 << 0):  # PF_X
                flags |= 0x08  # PTE.X
            if phdr.flags & (1 << 1):  # PF_W
                flags |= 0x04  # PTE.W
            if phdr.flags & (1 << 2):  # PF_R
                flags |= 0x02  # PTE.R
            if phdr.filesz != phdr.memsz:
                raise Exception("generate_bootstub.py does not yet support .bss")
            for offset in range(0, phdr.filesz, 4096):
                pgtbls.map(
                    kernel_start + phdr.offset + offset, phdr.vaddr + offset, flags
                )

    # Advance past the kernel, so next_paddr is pointing at the start of the
    # page tables.
    next_paddr += args.kernel.stat().st_size  # length of .kernel
    next_paddr = (next_paddr + 4095) & ~4095  # align up

    # Map the root page table to a known address. This relies on the root page
    # table being the first one we write out to the .kernel section.
    # Pgtbls.all_pgtbls() ensures this.
    pgtbls.map(
        next_paddr,
        0xFFFFFFFFFFFFD000,
        0xC7,  # PTE.D | PTE.A | PTE.W | PTE.R | PTE.V
    )

    # Compute the addresses of the page tables.
    pgtbl_paddrs = {}
    for addr, pgtbl in pgtbls.all_pgtbls():
        name = f"boot_pgtbl_{pgtbl.level}_{addr:016x}"
        pgtbl_paddrs[name] = next_paddr
        next_paddr += 4096

    # Write to a temporary file, so that a crash partway through doesn't
    # confuse Make into thinking we successfully completed next run.
    with NamedTemporaryFile("w") as tmp:
        # Embed the kernel.
        print(
            f"""
            .section .kernel, "a", @progbits
            .incbin "{args.kernel}"
            """,
            file=tmp,
        )

        # Embed the page tables.
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
                    entry = hex((child.ppn << 10) | child.flags)
                else:
                    assert isinstance(child, Pgtbl)
                    assert child.level == pgtbl.level - 1
                    child_name = f"boot_pgtbl_{child.level}_{child_addr:016x}"
                    entry = hex((pgtbl_paddrs[child_name] >> 2) | 0x01)
                entries.append(entry)
            print(f"    .8byte {', '.join(entries)}", file=tmp)

        # Copy the temporary file to the output file.
        tmp.flush()
        shutil.copy(tmp.name, args.bootstub)


if __name__ == "__main__":
    main()
