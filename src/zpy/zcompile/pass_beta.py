# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa


def pass_beta(func: ssa.Func):
    """
    A handful of beta-reductions (destructors applied to constructors).
    """

    replacements: dict[ssa.Insn, ssa.Insn] = {}
    # TODO: If we do this as a pre-order traversal on the dominator tree, we
    # only need one loop.
    for block in func:
        for insn in block:
            match insn:
                case ssa.InsnValueListGet((ssa.InsnValueListMake(values),), i):
                    replacements[insn] = values[i]
    for block in func:
        for insn in block:
            for i, arg in enumerate(insn.args):
                if arg in replacements:
                    insn.args[i] = replacements[arg]
