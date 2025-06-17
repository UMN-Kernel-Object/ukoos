#!/usr/bin/env python3

from argparse import ArgumentParser
from pathlib import Path

def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument("kernel", type=Path)
    arg_parser.add_argument("bootstub", type=Path)
    args = arg_parser.parse_args()

    with args.bootstub.open("w") as f:
        print(f"""
        .section .kernel
        .incbin "{args.kernel}"

        .section .bss
        """, file=f)

if __name__ == "__main__":
    main()
