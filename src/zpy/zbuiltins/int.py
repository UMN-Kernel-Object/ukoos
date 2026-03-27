# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from .helpers import defbuiltin
from zval import ZVal


@defbuiltin("%ZPY", "INT/+")
def int_add(args: tuple[ZVal, ...]) -> tuple[ZVal, ...]:
    l, r = args
    assert isinstance(l, int) and isinstance(r, int)
    return (l + r,)
