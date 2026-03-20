# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa
from .pass_never_elim import pass_never_elim


def optimize(func: ssa.Func):
    pass_never_elim(func)
