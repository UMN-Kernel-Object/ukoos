# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from .helpers import defbuiltin, defvar
from zval import NIL, ZVal


@defbuiltin("%ZPY", "DBG")
def dbg(xs: tuple[ZVal, ...]) -> tuple[ZVal, ...]:
    print(*xs)
    return xs


defvar("%ZPY", "OPTIMIZER/LOG?", NIL)


def ensure():
    import zbuiltins.int
