# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa


def analysis_users(func: ssa.Func) -> dict[ssa.Insn, set[tuple[int, ssa.Insn]]]:
    """
    Returns a mapping from instructions to the set of instructions that use the
    result of that instruction and the index the insn appears at.

    Note that an upsilon is not considered a user of its phi.
    """

    users = {insn: set() for block in func for insn in block}
    for block in func:
        for insn in block:
            for i, arg in enumerate(insn.args):
                users[arg].add((i, insn))
    return users
