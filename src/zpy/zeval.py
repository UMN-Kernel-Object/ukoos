# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later


from dataclasses import dataclass
import ssa
from zval import ZFunc, ZSym, ZVal


def apply(func: ZVal, *args: ZVal) -> tuple[ZVal, ...]:
    match func:
        case ZFunc(ssa_func):
            return apply_func(ssa_func, *args)
        case _:
            raise Exception(f"apply {func} {args}")


@dataclass
class ZValBox:
    value: ZVal


SSAValue = ZVal | tuple[ZVal, ...] | ZValBox


def apply_func(func: ssa.Func, *args: ZVal) -> tuple[ZVal, ...]:
    ssa_storage: dict[ssa.Insn, SSAValue] = {}
    insn = func.entry[0]
    while True:
        match insn:
            case ssa.InsnBranchEq((l, r), (if_t, if_f)):
                l = ssa_storage[l]
                r = ssa_storage[r]
                assert isinstance(l, ZVal) and isinstance(r, ZVal)
                insn = (if_t if l is r else if_f)[0]
                continue
            case ssa.InsnBranchIntEQ((l, r), (if_t, if_f)):
                l = ssa_storage[l]
                r = ssa_storage[r]
                assert isinstance(l, int) and isinstance(r, int)
                insn = (if_t if l == r else if_f)[0]
                continue
            case ssa.InsnConst((), value):
                ssa_storage[insn] = value
            case ssa.InsnGetArgs():
                ssa_storage[insn] = args
            case ssa.InsnGoto((), (dst,)):
                insn = dst[0]
                continue
            case ssa.InsnMakeValueList((value,)):
                value = ssa_storage[value]
                assert isinstance(value, ZVal)
                ssa_storage[insn] = (value,)
            case ssa.InsnPhi():
                pass
            case ssa.InsnRet((value_list,)):
                value_list = ssa_storage[value_list]
                assert isinstance(value_list, tuple)
                return value_list
            case ssa.InsnSymbolFunction((symbol,)):
                symbol = ssa_storage[symbol]
                assert isinstance(symbol, ZSym)
                assert symbol.function is not None, f"{symbol} did not have a function"
                ssa_storage[insn] = symbol.function
            case ssa.InsnUpsilon((value,), phi):
                ssa_storage[phi] = ssa_storage[value]
            case ssa.InsnValueListLength((value_list,)):
                value_list = ssa_storage[value_list]
                assert isinstance(value_list, tuple)
                ssa_storage[insn] = len(value_list)
            case _:
                print(func, file=__import__("sys").stderr)
                print(
                    {insn.name: value for insn, value in ssa_storage.items()},
                    file=__import__("sys").stderr,
                )
                raise Exception(f"TODO: {insn!r}")
        insn = insn.block[insn.parent_index + 1]
