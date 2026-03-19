# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""
An SSA-based IR in phi/upsilon form for interpreting (pre-)Z code.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Literal, Self
from zval import NIL, ZSym, ZVal


class Namer:
    def __init__(self, prefix: str):
        self.index = 0
        self.prefix = prefix

    def __call__(self) -> str:
        i = self.index
        self.index += 1
        return f"{self.prefix}{i}"


default_block_name = Namer("$")
default_insn_name = Namer("%")


Type = Literal["never", "unit", "value", "value-list", "value-ptr"]


class Insn(ABC):
    insn_name: str
    insn_ret_ty: Type
    insn_arg_tys: tuple[Type, ...]
    insn_vararg_ty: Type | None

    name: str
    args: tuple["Insn", ...]
    parent: "Block | None"

    def __init__(self, *args: "Insn"):
        assert type(self) is not Insn
        self.name = default_insn_name()
        self.args = args
        self.parent = None
        self.tyck()

    @property
    def func(self) -> "Func":
        assert self.parent is not None
        return self.parent.parent

    def tyck(self):
        cls = type(self)
        if cls.insn_vararg_ty is None:
            assert len(self.args) == len(cls.insn_arg_tys)
        else:
            assert len(self.args) >= len(cls.insn_arg_tys)
            for arg in self.args[len(cls.insn_arg_tys) :]:
                assert arg.type == cls.insn_vararg_ty

        for arg, ty in zip(self.args, cls.insn_arg_tys):
            assert arg.type == ty

    @property
    def type(self) -> Type:
        return type(self).insn_ret_ty

    def __repr__(self) -> str:
        insn_name = type(self).insn_name
        out = f"{self.name} <- {insn_name}"
        for i, arg in enumerate(self.args):
            if i != 0:
                out += ","
            out += " "
            out += arg.name
        return out


class InsnAbsurdList(Insn):
    insn_name = "absurd-list"
    insn_ret_ty = "value-list"
    insn_arg_tys = ("never",)
    insn_vararg_ty = None


class InsnCall(Insn):
    insn_name = "call"
    insn_ret_ty = "value-list"
    insn_arg_tys = ("value", "value-list")
    insn_vararg_ty = None


class InsnConst(Insn):
    insn_name = "const"
    insn_ret_ty = "value"
    insn_arg_tys = ()
    insn_vararg_ty = None

    value: ZVal

    def __init__(self, value: ZVal):
        self.value = value
        super().__init__()

    def __repr__(self):
        return f"{self.name} <- const {self.value}"


class InsnGetValue(Insn):
    insn_name = "get-value"
    insn_ret_ty = "value"
    insn_arg_tys = ("value-list",)
    insn_vararg_ty = None

    i: int

    def __init__(self, value: Insn, i: int):
        self.i = i
        super().__init__(value)

    def __repr__(self):
        return f"{self.name} <- get-value {self.args[0].name}, {self.i}"


class InsnMakeValueList(Insn):
    insn_name = "make-value-list"
    insn_ret_ty = "value-list"
    insn_arg_tys = ()
    insn_vararg_ty = "value"


class InsnPhi(Insn):
    insn_name = "phi"
    insn_ret_ty = "value"
    insn_arg_tys = ()
    insn_vararg_ty = None

    def new_upsilon(self, value: Insn) -> "InsnUpsilon":
        return InsnUpsilon(value, self)


class InsnRet(Insn):
    insn_name = "ret"
    insn_ret_ty = "never"
    insn_arg_tys = ("value-list",)
    insn_vararg_ty = None


class InsnSymbolFunction(Insn):
    insn_name = "symbol-function"
    insn_ret_ty = "value"
    insn_arg_tys = ("value",)
    insn_vararg_ty = None


class InsnTailCall(Insn):
    insn_name = "tailcall"
    insn_ret_ty = "never"
    insn_arg_tys = ("value", "value-list")
    insn_vararg_ty = None


class InsnUpsilon(Insn):
    insn_name = "upsilon"
    insn_ret_ty = "unit"
    insn_arg_tys = ("value",)
    insn_vararg_ty = None

    def __init__(self, value: Insn, shadow: Insn):
        assert isinstance(shadow, InsnPhi)
        self.shadow = shadow
        super().__init__(value)

    def __repr__(self):
        return f"{self.name} <- upsilon {self.args[0]}, ^{self.shadow}"


@dataclass(init=False)
class Block:
    name: str
    insns: list[Insn]
    parent: "Func"

    def __init__(self, parent: "Func", *, name: str | None = None):
        if name is None:
            name = default_block_name()
        self.name = name
        self.insns = []
        self.parent = parent

    def add[T: Insn](self, insn: T) -> T:
        insn.parent = self
        self.insns.append(insn)
        return insn

    def tyck(self):
        for insn in self.insns:
            insn.tyck()

    def __repr__(self) -> str:
        out = f"{self.name}:"
        for insn in self.insns:
            out += f"\n\t{insn!r}"
        return out


@dataclass(init=False)
class Func:
    name: ZSym
    blocks: list[Block]

    def __init__(self, name: ZSym = NIL):
        self.name = name
        self.blocks = []
        self.new_block()  # entry block

    def new_block(self) -> Block:
        block = Block(parent=self)
        self.blocks.append(block)
        return block

    def tyck(self):
        for block in self.blocks:
            block.tyck()

    def __repr__(self) -> str:
        out = f"func {self.name} {{\n"
        for block in self.blocks:
            out += f"{block!r}\n"
        out += "}"
        return out
