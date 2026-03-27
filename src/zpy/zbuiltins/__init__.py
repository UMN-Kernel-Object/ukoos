# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from .helpers import defbuiltin
from zval import ZVal


@defbuiltin("%ZPY", "DBG")
def dbg(xs: tuple[ZVal, ...]) -> tuple[ZVal, ...]:
    print(*xs)
    return xs


def ensure():
    import zbuiltins.int
