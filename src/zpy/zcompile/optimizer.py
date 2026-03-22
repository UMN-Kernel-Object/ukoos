# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa
from .pass_beta import pass_beta
from .pass_never_elim import pass_never_elim
from .pass_unused_elim import pass_unused_elim


def optimize(func: ssa.Func):
    pass_never_elim(func)
    pass_beta(func)
    pass_unused_elim(func)
