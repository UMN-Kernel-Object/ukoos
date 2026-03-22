# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa
from .analysis_users import analysis_users


def pass_unused_elim(func: ssa.Func):
    """
    Deletes any instructions that have no users and don't perform effects.
    """

    users = analysis_users(func)
    deleted = set()

    def should_delete(insn: ssa.Insn) -> bool:
        # Don't eliminate terminators; pass_never_elim will do that if it's
        # sound.
        if insn.jumps_to is not None:
            return False
        # Don't eliminate phis; their upsilons need to be deleted at the same time.
        if isinstance(insn, ssa.InsnPhi):
            return False
        # Don't eliminate instructions that perform effects.
        for effect in insn.effects():
            if effect.rw != "r":
                return False
        # Don't eliminate instructions that get used.
        for _, user in users[insn]:
            if user not in deleted:
                return False

        # Otherwise, we can plan to delete the instruction.
        return True

    # Save the list of blocks to avoid iterator invalidation.
    blocks = list(func.dom.postorder())
    for block in blocks:
        indices = []
        for i, insn in enumerate(reversed(block)):
            if should_delete(insn):
                deleted.add(insn)
                indices.append(len(block) - i - 1)
        block.remove_all(indices)
