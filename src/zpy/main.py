#!/usr/bin/env python3

# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from pathlib import Path
from sys import argv
from reader import read_one_from_string
from zcompile import GlobalEnv, zcompile
from zeval import apply
from zval import NIL, ZCons, ZStr, ZSym, ZVal


def main(src_path: Path, *args: str):
    load(src_path)


def load(path: Path):
    with path.open() as f:
        src = ZStr(f.read())

    while True:
        form, new_src = read_one_from_string(src)
        if new_src is None:
            break
        src = new_src

        zeval_toplevel(form)


def zeval_toplevel(form: ZVal) -> tuple[ZVal, ...]:
    """
    The evaluator.

    This always runs in the global lexical environment, and so is a thin
    wrapper around the compiler.
    """

    # Wrap the form in a lambda and compile it.
    form = ZCons.of_list(ZSym.impl("LAMBDA"), NIL, ZCons.of_list(), form)
    func = zcompile(form, GlobalEnv())

    raise Exception(f"zeval_toplevel {func}")


if __name__ == "__main__":
    main(Path(argv[1]), *argv[2:])
