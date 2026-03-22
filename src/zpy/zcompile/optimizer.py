# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa
from .pass_beta import pass_beta
from .pass_jump_simplify import pass_jump_simplify
from .pass_unused_elim import pass_unused_elim


def optimize(func: ssa.Func):
    func.tyck()
    pass_beta(func)
    pass_unused_elim(func)
    pass_jump_simplify(func)
    pass_unused_elim(func)
    func.tyck()
