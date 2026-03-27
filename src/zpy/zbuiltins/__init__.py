# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from .helpers import defbuiltin, defvar, export
from zval import NIL, ZSym, ZVal


@defbuiltin("%ZPY", "DBG")
def dbg(xs: tuple[ZVal, ...]) -> tuple[ZVal, ...]:
    print(*xs)
    return xs


defvar("%ZPY", "OPTIMIZER/LOG?", NIL)

export(
    ZSym.z("FUNCTION"),
    ZSym.z("IF"),
    ZSym.impl("LAMBDA"),
    ZSym.z("LET"),
    ZSym.z("PROGN"),
    ZSym.z("QUOTE"),
)


def ensure():
    import zbuiltins.int
    import zbuiltins.sym
