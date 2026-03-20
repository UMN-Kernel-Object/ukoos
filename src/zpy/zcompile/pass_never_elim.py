# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa


def pass_never_elim(func: ssa.Func):
    """
    Deletes any instructions that come after an instruction returning never.
    """

    for block in func.blocks:
        on_block(block)


def on_block(block: ssa.Block):
    for i, insn in enumerate(block.insns):
        if insn.type == "never":
            print(block.remove_all(list(range(i + 1, len(block.insns)))))
