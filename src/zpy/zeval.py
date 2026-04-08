# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later


from dataclasses import dataclass
import ssa
from zval import ZFunc, ZSym, ZVal


@dataclass
class ZValBox:
    value: ZVal


SSAValue = ZVal | tuple[ZVal, ...] | ZValBox


def _ensure_one(value: SSAValue) -> ZVal:
    assert isinstance(value, ZVal)
    return value


@dataclass
class CallFrame:
    func: ssa.Func
    insn: ssa.Insn
    args: tuple[ZVal, ...]
    ssa_storage: dict[ssa.Insn, SSAValue]


SSAFrame = CallFrame


def call(func: ssa.Func, *args: ZVal) -> tuple[ZVal, ...]:
    stack: list[SSAFrame] = []
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

            case ssa.InsnBuiltin((call_args,), call_func):
                call_args = ssa_storage[call_args]
                ssa_storage[insn] = call_func(call_args)

            case ssa.InsnCall((call_func, call_args)):
                call_func = ssa_storage[call_func]
                call_args = ssa_storage[call_args]
                assert isinstance(call_func, ZFunc)
                assert isinstance(call_args, tuple)

                stack.append(CallFrame(func, insn, args, ssa_storage))
                func = call_func.func
                args = call_args
                ssa_storage = {}
                insn = func.entry[0]

                continue

            case ssa.InsnConst((), value):
                ssa_storage[insn] = value

            case ssa.InsnGetArgs():
                ssa_storage[insn] = args

            case ssa.InsnGoto((), (dst,)):
                insn = dst[0]
                continue

            case ssa.InsnPhi():
                pass

            case ssa.InsnRet((value_list,)):
                value_list = ssa_storage[value_list]
                assert isinstance(value_list, tuple)
                if len(stack) == 0:
                    return value_list
                else:
                    match stack.pop():
                        case CallFrame(ret_func, ret_insn, ret_args, ret_ssa_storage):
                            func = ret_func
                            args = ret_args
                            ssa_storage = ret_ssa_storage
                            insn = ret_insn
                            ssa_storage[ret_insn] = value_list
                        case frame:
                            raise Exception(f"TODO: {frame}")

            case ssa.InsnSymbolFunction((symbol,)):
                symbol = ssa_storage[symbol]
                assert isinstance(symbol, ZSym)
                assert symbol.function is not None, f"{symbol} did not have a function"
                ssa_storage[insn] = symbol.function

            case ssa.InsnTailCall((call_func, call_args)):
                call_func = ssa_storage[call_func]
                call_args = ssa_storage[call_args]
                assert isinstance(call_func, ZFunc)
                assert isinstance(call_args, tuple)

                func = call_func.func
                args = call_args
                ssa_storage = {}
                insn = func.entry[0]

                continue

            case ssa.InsnUpsilon((value,), phi):
                ssa_storage[phi] = ssa_storage[value]

            case ssa.InsnValueListGet((value_list,), i):
                value_list = ssa_storage[value_list]
                assert isinstance(value_list, tuple)
                assert i < len(value_list)
                ssa_storage[insn] = value_list[i]

            case ssa.InsnValueListLength((value_list,)):
                value_list = ssa_storage[value_list]
                assert isinstance(value_list, tuple)
                ssa_storage[insn] = len(value_list)

            case ssa.InsnValueListMake(values):
                ssa_storage[insn] = tuple(
                    _ensure_one(ssa_storage[value]) for value in values
                )

            case _:
                print(func, file=__import__("sys").stderr)
                print(
                    {insn.name: value for insn, value in ssa_storage.items()},
                    file=__import__("sys").stderr,
                )
                raise Exception(f"TODO: {insn!r}")

        insn = insn.block[insn.parent_index + 1]
