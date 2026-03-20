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
from .optimizer import optimize


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
    decls: dict[ZSym, Decls]
    funcs: dict[ZSym, tuple[FuncBinding | None, Decls]]
    vars: dict[ZSym, tuple[VarBinding | None, Decls]]

    def __init__(self, parent: Env):
        self.parent = parent
        self.decls = {}
        self.funcs = {}
        self.vars = {}

    def add_lexical_var(self, sym: ZSym, alloca: ssa.Insn):
        assert not sym.is_keyword
        raise Exception("LexicalEnv.add_lexical_var")

    def find_decl(self, sym: ZSym) -> Decls:
        if sym in self.decls:
            return self.decls[sym]
        else:
            return self.parent.find_decl(sym)

    def find_func(self, sym: ZSym) -> tuple[FuncBinding | None, Decls]:
        if sym in self.funcs:
            return self.funcs[sym]
        else:
            return self.parent.find_func(sym)

    def find_var(self, sym: ZSym) -> tuple[VarBinding | None, Decls]:
        if sym in self.vars:
            return self.vars[sym]
        else:
            return self.parent.find_var(sym)


@dataclass
class IRBuilder:
    block: ssa.Block
    env: Env

    def copy_from(self, other: "IRBuilder") -> None:
        self.block = other.block
        self.env = other.env

    def copy_with(
        self,
        *,
        block: ssa.Block | None = None,
        env: Env | None = None,
    ) -> "IRBuilder":
        if block is None:
            block = self.block
        if env is None:
            env = self.env
        return IRBuilder(block, env)

    def absurd_list(self, never: ssa.Insn) -> ssa.InsnAbsurdList:
        return self.block.add(ssa.InsnAbsurdList(never))

    def alloca(self, name: ZSym) -> ssa.InsnAlloca:
        insn = ssa.InsnAlloca()
        insn.name = f"lexical({name})"
        return self.block.add(insn)

    def branch_eq(
        self,
        lhs: ssa.Insn,
        rhs: ssa.Insn,
        *,
        if_eq: ssa.Block | None = None,
        if_ne: ssa.Block | None = None,
    ) -> tuple[ssa.InsnBranchEq, "IRBuilder", "IRBuilder"]:
        if if_eq is None:
            if_eq = self.block.parent.new_block()
        if if_ne is None:
            if_ne = self.block.parent.new_block()
        return (
            self.block.add(ssa.InsnBranchEq(lhs, rhs, if_eq, if_ne)),
            self.copy_with(block=if_eq),
            self.copy_with(block=if_ne),
        )

    def branch_int_eq(
        self,
        lhs: ssa.Insn,
        rhs: ssa.Insn,
        *,
        if_eq: ssa.Block | None = None,
        if_ne: ssa.Block | None = None,
    ) -> tuple[ssa.InsnBranchIntEQ, "IRBuilder", "IRBuilder"]:
        if if_eq is None:
            if_eq = self.block.parent.new_block()
        if if_ne is None:
            if_ne = self.block.parent.new_block()
        return (
            self.block.add(ssa.InsnBranchIntEQ(lhs, rhs, if_eq, if_ne)),
            self.copy_with(block=if_eq),
            self.copy_with(block=if_ne),
        )

    def branch_int_lt(
        self,
        lhs: ssa.Insn,
        rhs: ssa.Insn,
        *,
        if_lt: ssa.Block | None = None,
        if_ge: ssa.Block | None = None,
    ) -> tuple[ssa.InsnBranchIntLT, "IRBuilder", "IRBuilder"]:
        if if_lt is None:
            if_lt = self.block.parent.new_block()
        if if_ge is None:
            if_ge = self.block.parent.new_block()
        return (
            self.block.add(ssa.InsnBranchIntLT(lhs, rhs, if_lt, if_ge)),
            self.copy_with(block=if_lt),
            self.copy_with(block=if_ge),
        )

    def call(self, func: ssa.Insn, args: ssa.Insn) -> ssa.InsnCall:
        return self.block.add(ssa.InsnCall(func, args))

    def const(self, value: ZVal) -> ssa.InsnConst:
        return self.block.add(ssa.InsnConst(value))

    def get_args(self) -> ssa.InsnGetArgs:
        return self.block.add(ssa.InsnGetArgs())

    def get_value(self, value_list: ssa.Insn, i: int) -> ssa.InsnGetValue:
        return self.block.add(ssa.InsnGetValue(value_list, i))

    def make_value_list(self, *values: ssa.Insn) -> ssa.InsnMakeValueList:
        return self.block.add(ssa.InsnMakeValueList(*values))

    def phi(self) -> ssa.InsnPhi:
        return self.block.add(ssa.InsnPhi())

    def ptr_read(self, ptr: ssa.Insn) -> ssa.InsnPtrRead:
        return self.block.add(ssa.InsnPtrRead(ptr))

    def ptr_write(self, ptr: ssa.Insn, val: ssa.Insn) -> ssa.InsnPtrWrite:
        return self.block.add(ssa.InsnPtrWrite(ptr, val))

    def ret(self, values: ssa.Insn) -> ssa.InsnRet:
        return self.block.add(ssa.InsnRet(values))

    def symbol_function(self, symbol: ssa.Insn) -> ssa.InsnSymbolFunction:
        return self.block.add(ssa.InsnSymbolFunction(symbol))

    def tail_call(self, func: ssa.Insn, args: ssa.Insn) -> ssa.InsnTailCall:
        return self.block.add(ssa.InsnTailCall(func, args))

    def unreachable(self) -> ssa.InsnUnreachable:
        return self.block.add(ssa.InsnUnreachable())

    def upsilon(self, value: ssa.Insn, shadow: ssa.Insn) -> ssa.InsnUpsilon:
        return self.block.add(ssa.InsnUpsilon(value, shadow))

    def value_list_length(self, values: ssa.Insn) -> ssa.InsnValueListLength:
        return self.block.add(ssa.InsnValueListLength(values))


@dataclass(init=False)
class LambdaList:
    required: list[ZSym]
    optional: list[ZSym]
    rest: ZSym | None

    def __init__(self, lambda_list: ZVal):
        self.required = []
        self.optional = []
        self.rest = None

        i = 0
        args = ZCons.collect(lambda_list)

        # Accept required parameters.
        while i < len(args):
            arg = args[i]
            assert isinstance(arg, ZSym)
            if arg.is_keyword:
                raise Exception(f"LambdaList {args[i:]}")
            else:
                self.required.append(arg)
                i += 1

        # Check that there are no leftover parameters.
        if i < len(args):
            arg = args[i]
            raise Exception(f"unexpected parameter {arg} in lambda list {lambda_list}")

    def compile(self, ir: IRBuilder) -> LexicalEnv:
        """
        Compiles code to take apart the function's argument list and match it
        against the lambda list, pushing and returning a lexical environment.
        """

        env = LexicalEnv(ir.env)
        length = ir.value_list_length(ir.block.parent.args)
        expected_length = 0

        for arg in self.required:
            raise Exception(f"LambdaList.compile {self}")

        for arg in self.optional:
            raise Exception(f"LambdaList.compile {self}")

        if self.rest is None:
            _, if_eq, if_ne = ir.branch_int_eq(length, ir.const(expected_length))
            ir.copy_from(if_eq)
            # TODO
            if_ne.unreachable()
        else:
            raise Exception(f"LambdaList.compile {self}")

        ir.env = env
        return env


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

    # Compile the argument list.
    args_env = LambdaList(args).compile(ir)
    assert ir.env is args_env

    # Compile the body.
    return_values = compile_progn(ir, body_init, body_last, tail=True)

    # Add a return to the exit block.
    ir.ret(return_values)

    # Check that the IR builder is in the state we expect.
    assert ir.env is args_env
    ir.env = args_env.parent
    assert ir.env is env

    # Type-check the function.
    func.tyck()
    assert return_values.type == "value-list"

    # Run the optimizer.
    print(form)
    print(func)
    optimize(func)
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
