# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa


def analysis_uses(func: ssa.Func) -> dict[ssa.Insn, set[ssa.Insn]]:
    """
    Returns a mapping from instructions to the set of instructions that the
    instruction uses, either directly as an argument or indirectly through
    effects.
    """

    block_effects: list[dict[str, set[ssa.Insn]]] = [
        {effect: set() for effect in func.effect_registry.effects} for _ in func
    ]
    insns: dict[ssa.Insn, set[ssa.Insn]] = {
        insn: set() for block in func for insn in block
    }

    # Iteratively compute the _uses_.
    changed = True
    while changed:
        changed = False
        for block in func:
            # Collect uses across the predecessors of the block.
            effects: dict[str, set[ssa.Insn]] = {
                effect: set() for effect in func.effect_registry.effects
            }
            for pred in func.preds(block):
                effects_in = block_effects[pred.parent_index]
                for effect in func.effect_registry.effects:
                    effects[effect] |= effects_in[effect]

            for insn in block:
                uses = set()

                # We inherit all the uses of our arguments.
                for arg in insn.args:
                    uses.add(arg)
                    uses |= insns[arg]

                # If we read from an effect, we use anything that affected it.
                for effect in insn.effects():
                    if effect.reads:
                        uses |= effects[effect.name]

                # If we write to an effect, we affect it and anything that
                # descends from it.
                for effect in insn.effects():
                    if effect.writes:
                        for effect in func.effect_registry.descendants(effect.name):
                            effects[effect].add(insn)
                            effects[effect] |= uses

                if insns[insn] != uses:
                    insns[insn] = uses
                    changed = True

            # Save this as our effects.
            if block_effects[block.parent_index] != effects:
                block_effects[block.parent_index] = effects
                changed = True

    return insns
