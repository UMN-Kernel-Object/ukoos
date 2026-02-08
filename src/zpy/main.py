#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from pathlib import Path
from sys import argv
from zval import ModuleZ


def main(src_path: Path, *args: str):
    with src_path.open() as src_file:
        src = src_file.read()

    pass


if __name__ == "__main__":
    main(Path(argv[0]), *argv[1:])
