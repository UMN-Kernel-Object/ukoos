# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

import ssa
from typing import Callable
from zcompile import GlobalEnv, IRBuilder
from zval import ZCons, ZFunc, ZMod, ZSym, ZVal


def defbuiltin(mod_name: str, name: str, *, export: bool = True):
    mod = ZMod.find(mod_name)
    sym, sym_status = mod.intern(name)
    assert sym_status != "INHERITED"
    if export:
        mod.export(sym)

    def decorator(builtin: Callable[[tuple[ZVal, ...]], tuple[ZVal, ...]]):
        lambda_list = ZCons.of_list()

        func = ssa.Func(sym, ZSym.keyword("BUILTIN"))
        ir = IRBuilder(func.entry, GlobalEnv())
        ir.ret(ir.builtin(builtin, func.args))
        func.tyck()

        assert sym.function is None
        sym.function = ZFunc(func=func, lambda_list=lambda_list)

    return decorator


def defvar(mod_name: str, name: str, value: ZVal, *, export: bool = True):
    mod = ZMod.find(mod_name)
    sym, sym_status = mod.intern(name)
    assert sym_status != "INHERITED"
    if export:
        mod.export(sym)

    assert sym.value is None
    sym.value = value


def export(*syms: ZSym):
    for sym in syms:
        sym.mod.export(sym)
