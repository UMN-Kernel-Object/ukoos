# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from contextlib import ExitStack
from pathlib import Path
import ssa
import subprocess
from sys import stdout
from tempfile import NamedTemporaryFile
from .pass_beta import pass_beta
from .pass_jump_simplify import pass_jump_simplify
from .pass_unused_elim import pass_unused_elim


def optimize(func: ssa.Func, *, log: bool = False):
    passes = [
        pass_beta,
        pass_unused_elim,
        pass_jump_simplify,
        pass_unused_elim,
    ]

    func.tyck()

    if log:
        print("Before optimizer:", func)

    before_str = ""
    for pass_func in passes:
        if log:
            before_str = str(func)
        pass_func(func)
        if log:
            log_pass(pass_func.__name__, before_str, str(func))
        func.tyck()


def log_pass(pass_name: str, before: str, after: str):
    with ExitStack() as stack:

        def save(s: str) -> Path:
            file = stack.push(NamedTemporaryFile("w"))
            print(s, file=file)
            file.flush()
            return Path(file.name)

        before_path = save(before)
        after_path = save(after)

        approaches = [
            lambda: subprocess.run(
                [
                    "delta",
                    "--detect-dark-light=never",
                    "--paging=never",
                    before_path,
                    after_path,
                ],
                stdout=stdout,
                stderr=subprocess.STDOUT,
            ),
            lambda: subprocess.run(
                ["diff", "-u", before_path, after_path],
                stdout=stdout,
                stderr=subprocess.STDOUT,
            ),
            lambda: print(after),
        ]

        print(f"After {pass_name}:")
        stdout.flush()
        for approach in approaches:
            try:
                approach()
                return
            except FileNotFoundError:
                continue
