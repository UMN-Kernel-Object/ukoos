# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from .helpers import defbuiltin
from zval import ZSym, ZVal


@defbuiltin("%ZPY", "SYM/SET-FUNCTION")
def sym_set_function(args: tuple[ZVal, ...]) -> tuple[ZVal, ...]:
    sym, func = args
    assert isinstance(sym, ZSym)
    sym.function = func
    return ()


@defbuiltin("%ZPY", "SYM/SET-MACRO")
def sym_set_macro(args: tuple[ZVal, ...]) -> tuple[ZVal, ...]:
    sym, macro = args
    assert isinstance(sym, ZSym)
    sym.macro = macro
    return ()


@defbuiltin("%ZPY", "SYM/SET-SETF")
def sym_set_setf(args: tuple[ZVal, ...]) -> tuple[ZVal, ...]:
    sym, setf = args
    assert isinstance(sym, ZSym)
    sym.setf = setf
    return ()


@defbuiltin("%ZPY", "SYM/SET-VALUE")
def sym_set_value(args: tuple[ZVal, ...]) -> tuple[ZVal, ...]:
    sym, value = args
    assert isinstance(sym, ZSym)
    sym.value = value
    return ()
