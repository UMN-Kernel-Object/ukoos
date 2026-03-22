# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa
from .analysis_uses import analysis_uses


def pass_unused_elim(func: ssa.Func):
    """
    Deletes any instructions that are not used to compute the function's result.
    """

    uses = analysis_uses(func)

    # Keep any instruction used by a block that exits the function.
    keep: set[ssa.Insn] = set()
    for block in func:
        keep.add(block.terminator)
        if block.exit:
            keep |= uses[block.terminator]
    for block in func:
        block.remove_all([i for i, insn in enumerate(block) if insn not in keep])
