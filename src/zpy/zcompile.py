# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""
The compiler.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
import ssa
from typing import Literal
from zval import NIL, ZCons, ZSym, ZVal


Decls = list[tuple[ZSym, ZVal]]
FuncBinding = Literal["GLOBAL", "MACRO", "SPECIAL-FORM"] | ssa.Insn
VarBinding = Literal["GLOBAL", "SYMBOL-MACRO"] | ssa.Insn

SPECIAL_FORMS = [
    ZSym.z("FUNCTION"),
    ZSym.z("LET"),
    ZSym.z("PROGN"),
]


class Env(ABC):
    def extend(self) -> "Env":
        return LexicalEnv(self)

    @abstractmethod
    def find_decl(self, sym: ZSym) -> Decls: ...

    @abstractmethod
    def find_func(self, sym: ZSym) -> tuple[FuncBinding | None, Decls]: ...

    @abstractmethod
    def find_var(self, sym: ZSym) -> tuple[VarBinding | None, Decls]: ...


class GlobalEnv(Env):
    instance: "GlobalEnv | None" = None
    inited: bool = False

    def __new__(cls) -> "GlobalEnv":
        if cls.instance is None:
            cls.instance = super().__new__(cls)
        return cls.instance

    def __init__(self):
        if self.inited:
            return
        self.inited = True

    def find_decl(self, sym: ZSym) -> Decls:
        assert not sym.is_keyword
        raise Exception("GlobalEnv.find_decl")

    def find_func(self, sym: ZSym) -> tuple[FuncBinding | None, Decls]:
        assert not sym.is_keyword
        if sym in SPECIAL_FORMS:
            return "SPECIAL-FORM", []
        elif sym.macro is not None:
            # TODO: decls
            return "MACRO", []
        elif sym.function is not None:
            # TODO: decls
            return "GLOBAL", []
        else:
            return None, []

    def find_var(self, sym: ZSym) -> tuple[VarBinding | None, Decls]:
        assert not sym.is_keyword
        raise Exception("GlobalEnv.find_var")


class LexicalEnv(Env):
    parent: Env

    def __init__(self, parent: Env):
        self.parent = parent

    def find_decl(self, sym: ZSym) -> Decls:
        assert not sym.is_keyword
        raise Exception("LexicalEnv.find_decl")

    def find_func(self, sym: ZSym) -> tuple[FuncBinding | None, Decls]:
        assert not sym.is_keyword
        raise Exception("LexicalEnv.find_func")

    def find_var(self, sym: ZSym) -> tuple[VarBinding | None, Decls]:
        assert not sym.is_keyword
        raise Exception("LexicalEnv.find_var")


@dataclass
class IRBuilder:
    block: ssa.Block
    env: Env

    def absurd_list(self, never: ssa.Insn) -> ssa.InsnAbsurdList:
        return self.block.add(ssa.InsnAbsurdList(never))

    def call(self, func: ssa.Insn, args: ssa.Insn) -> ssa.InsnCall:
        return self.block.add(ssa.InsnCall(func, args))

    def const(self, value: ZVal) -> ssa.InsnConst:
        return self.block.add(ssa.InsnConst(value))

    def get_value(self, value_list: ssa.Insn, i: int) -> ssa.InsnGetValue:
        return self.block.add(ssa.InsnGetValue(value_list, i))

    def make_value_list(self, *values: ssa.Insn) -> ssa.InsnMakeValueList:
        return self.block.add(ssa.InsnMakeValueList(*values))

    def phi(self) -> ssa.InsnPhi:
        return self.block.add(ssa.InsnPhi())

    def ret(self, values: ssa.Insn) -> ssa.InsnRet:
        return self.block.add(ssa.InsnRet(values))

    def symbol_function(self, symbol: ssa.Insn) -> ssa.InsnSymbolFunction:
        return self.block.add(ssa.InsnSymbolFunction(symbol))

    def tail_call(self, func: ssa.Insn, args: ssa.Insn) -> ssa.InsnTailCall:
        return self.block.add(ssa.InsnTailCall(func, args))

    def upsilon(self, value: ssa.Insn, shadow: ssa.Insn) -> ssa.InsnUpsilon:
        return self.block.add(ssa.InsnUpsilon(value, shadow))


def zcompile(form: ZVal, env: Env) -> ssa.Func:
    """
    Compiles a function definition to an SSA function.
    """

    # Check that it's a lambda and take it apart.
    assert isinstance(form, ZCons)
    assert form.car is ZSym.impl("LAMBDA")
    assert isinstance(form.cdr, ZCons)
    assert isinstance(form.cdr.car, ZSym)
    func_name = form.cdr.car
    assert isinstance(form.cdr.cdr, ZCons)
    assert isinstance(form.cdr.cdr.car, ZCons) or form.cdr.cdr.car is NIL
    args = form.cdr.cdr.car
    assert isinstance(form.cdr.cdr.cdr, ZCons)
    body = form.cdr.cdr.cdr.collect()
    body_init = body[:-1]
    body_last = body[-1]

    # Make a builder pointing at the entry block.
    func = ssa.Func(name=func_name)
    ir = IRBuilder(func.blocks[0], env)

    # TODO: Compile the argument list.

    # Compile the body.
    return_values = compile_progn(ir, body_init, body_last, tail=True)

    # Add a return to the exit block.
    ir.ret(return_values)

    # Type-check the function.
    func.tyck()
    assert return_values.type == "value-list"

    # TODO: Run the optimizer.

    print(form)
    print(func)
    return func


def compile_form(ir: IRBuilder, form: ZVal, *, tail: bool = False) -> ssa.Insn:
    """
    Compiles an expression, returning the instruction that computes it as a
    value-list. Expands macros.
    """

    if isinstance(form, ZSym):
        raise Exception(f"TODO: compile_form {form}")
    elif isinstance(form, ZCons):
        if isinstance(form.car, ZSym):
            binding, _ = ir.env.find_func(form.car)
            if binding == "GLOBAL" or binding is None:
                func = ir.symbol_function(ir.const(form.car))
                return compile_call(ir, func, ZCons.collect(form.cdr), tail=tail)
            elif isinstance(binding, ssa.Insn):
                return compile_call(ir, binding, ZCons.collect(form.cdr), tail=tail)
            elif binding == "MACRO":
                raise Exception(f"TODO: compile_form {form}")
            elif binding == "SPECIAL-FORM":
                raise Exception(f"TODO: compile_form {form}")
            else:
                raise Exception(f"TODO: compile_form {form} {binding}")
        else:
            # TODO: Check that this is a lambda.
            raise Exception(f"TODO: compile_form {form}")
            # func = form.car
            # args = ZCons.collect(form.cdr)
            # return compile_call(ir, func, args)
    else:
        return ir.make_value_list(ir.const(form))


def compile_call(
    ir: IRBuilder, func: ssa.Insn, args: list[ZVal], *, tail: bool = False
) -> ssa.Insn:
    """
    Compiles a function call, returning the instruction that computes it as a
    value-list.
    """

    args_insn = ir.make_value_list(
        *(ir.get_value(compile_form(ir, arg), 0) for arg in args)
    )
    if tail:
        return ir.absurd_list(ir.tail_call(func, args_insn))
    else:
        return ir.call(func, args_insn)


def compile_progn(
    ir: IRBuilder, init: list[ZVal], last: ZVal, *, tail: bool = False
) -> ssa.Insn:
    """
    Compiles a progn with INIT initial forms and a LAST final form, returning
    the instruction that computes LAST as a value-list.
    """

    for form in init:
        compile_form(ir, form)
    return compile_form(ir, last, tail=tail)
