# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa


def pass_jump_simplify(func: ssa.Func):
    """
    Performs the following to a fixed point:

    - Jump threading: makes a jump to a goto instead just jump to the eventual
      destination.
    - Converts a branch that only jumps to a single destination into a goto to
      that destination.
    """

    changed = True
    while changed:
        changed = False
        for block in func:
            term = block.terminator

            # Jump threading.
            for i, dst in enumerate(term.jumps_to):
                if dst is block:
                    continue
                dst_term = dst.terminator
                if isinstance(dst_term, ssa.InsnGoto) and dst_term.parent_index == 0:
                    term.jumps_to[i] = dst_term.jumps_to[0]
                    func.invalidate()
                    changed = True

            # Convert a branch that always goes to the same destination into a
            # goto.
            if not isinstance(term, ssa.InsnGoto):
                if len(set(block.parent_index for block in term.jumps_to)) == 1:
                    block[term.parent_index] = ssa.InsnGoto(term.jumps_to[0])
                    changed = True
                    continue
