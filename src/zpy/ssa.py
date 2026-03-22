# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""
An SSA-based IR in phi/upsilon form for interpreting (pre-)Z code.

This is heavily based on [0] -- if you're curious why something is done a
certain way, check there first.

[0]: https://gist.github.com/pizlonator/cf1e72b8600b1437dda8153ea3fdb963
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Iterable, Literal
from zval import NIL, ZSym, ZVal


@dataclass
class EffectNode:
    preorder: int
    postorder: int
    children: list[str]


@dataclass(init=False)
class EffectRegistry:
    effects: dict[str, EffectNode]
    valid: bool

    def __init__(self):
        self.effects = {"world": EffectNode(-1, -1, [])}
        self.add_effect("control", parent="world")
        self.add_effect("memory", parent="world")
        self.add_effect("phi-upsilon", parent="world")
        self.add_effect("symbol-function", parent="world")
        self.validate()

    def __getitem__(self, name: str) -> tuple[int, int]:
        self.validate()
        effect = self.effects[name]
        return effect.preorder, effect.postorder

    def add_effect(self, name: str, *, parent: str):
        self.valid = False
        assert name not in self.effects
        self.effects[parent].children.append(name)
        self.effects[name] = EffectNode(-1, -1, [])

    def interferes(self, eff_a: "Effect", eff_b: "Effect") -> bool:
        self.validate()
        if eff_a.rw == "r" and eff_b.rw == "r":
            return False
        if eff_a.name == eff_b.name:
            return True
        a = self.effects[eff_a.name]
        b = self.effects[eff_b.name]
        return (a.preorder < b.preorder) != (a.postorder < b.postorder)

    def validate(self):
        if self.valid:
            return

        def traverse(node: str, pre: int, post: int) -> tuple[int, int]:
            self.effects[node].preorder = pre
            pre += 1
            for child in self.effects[node].children:
                pre, post = traverse(child, pre, post)
            self.effects[node].postorder = post
            post += 1
            return pre, post

        pre, post = traverse("world", 0, 0)
        assert pre == len(self.effects) and post == len(self.effects)
        self.valid = True


@dataclass
class Effect:
    name: str
    rw: Literal["r", "w"]


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
    args: list["Insn"]
    parent: "Block | None"

    def __init__(self, *args: "Insn"):
        assert type(self) is not Insn
        self.name = default_insn_name()
        self.args = list(args)
        self.parent = None
        self.tyck()

    @abstractmethod
    def effects(self) -> Iterable[Effect]: ...

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

        if self.parent is not None:
            for arg in self.args:
                assert self.parent is arg.parent

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

    def remove_all(self, indices: list[int]) -> list[Insn]:
        for i in indices:
            assert 0 <= i < len(self.insns)
        to_remove = frozenset(indices)
        removed: dict[int, Insn] = {}
        new_insns = []
        for i, insn in enumerate(self.insns):
            if (i - len(removed)) in to_remove:
                insn.parent = None
                removed[i] = insn
            else:
                new_insns.append(insn)

        self.insns.clear()
        for insn in new_insns:
            self.insns.append(insn)

        return [removed[i] for i in indices]

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
    args: Insn
    blocks: list[Block]
    effect_registry: EffectRegistry

    def __init__(self, name: ZSym = NIL):
        self.name = name
        self.blocks = []
        self.effect_registry = EffectRegistry()

        entry_block = self.new_block()
        self.args = entry_block.add(InsnGetArgs())

    def new_block(self) -> Block:
        block = Block(parent=self)
        self.blocks.append(block)
        return block

    def tyck(self):
        for block in self.blocks:
            block.tyck()

    def __repr__(self) -> str:
        out = f"func ({self.name} {self.args.name}) {{\n"
        for block in self.blocks:
            out += f"{block!r}\n"
        out += "}"
        return out


class InsnBranchBinop(Insn):
    """
    The common parent instruction of branching binops.
    """

    if_t: Block
    if_f: Block

    def __init__(self, lhs: Insn, rhs: Insn, if_t: Block, if_f: Block):
        self.if_t = if_t
        self.if_f = if_f
        super().__init__(lhs, rhs)

    def __repr__(self):
        cls = type(self)
        return f"{self.name} <- {cls.insn_name} {self.args[0].name}, {self.args[1].name}, {self.if_t.name}, {self.if_f.name}"

    def effects(self) -> Iterable[Effect]:
        yield Effect("control", "w")


class InsnPure(Insn):
    """
    A class for instructions with no effects.
    """

    def effects(self) -> Iterable[Effect]:
        return ()


class InsnAbsurdList(InsnPure):
    insn_name = "absurd-list"
    insn_ret_ty = "value-list"
    insn_arg_tys = ("never",)
    insn_vararg_ty = None
    __match_args__ = ("args",)


class InsnAlloca(InsnPure):
    insn_name = "alloca"
    insn_ret_ty = "value-ptr"
    insn_arg_tys = ()
    insn_vararg_ty = None
    __match_args__ = ("args",)


class InsnBranchEq(InsnBranchBinop):
    insn_name = "branch-eq"
    insn_ret_ty = "never"
    insn_arg_tys = ("value", "value")
    insn_vararg_ty = None
    __match_args__ = ("args", "if_t", "if_f")


class InsnBranchIntEQ(InsnBranchBinop):
    insn_name = "branch-int-eq"
    insn_ret_ty = "never"
    insn_arg_tys = ("value", "value")
    insn_vararg_ty = None
    __match_args__ = ("args", "if_t", "if_f")


class InsnBranchIntLT(InsnBranchBinop):
    insn_name = "branch-int-lt"
    insn_ret_ty = "never"
    insn_arg_tys = ("value", "value")
    insn_vararg_ty = None
    __match_args__ = ("args", "if_t", "if_f")


class InsnCall(Insn):
    insn_name = "call"
    insn_ret_ty = "value-list"
    insn_arg_tys = ("value", "value-list")
    insn_vararg_ty = None
    __match_args__ = ("args",)

    def effects(self) -> Iterable[Effect]:
        yield Effect("world", "w")


class InsnConst(InsnPure):
    insn_name = "const"
    insn_ret_ty = "value"
    insn_arg_tys = ()
    insn_vararg_ty = None
    __match_args__ = ("args", "value")

    value: ZVal

    def __init__(self, value: ZVal):
        self.value = value
        super().__init__()

    def __repr__(self):
        return f"{self.name} <- const {self.value}"


class InsnGetArgs(InsnPure):
    insn_name = "get-args"
    insn_ret_ty = "value-list"
    insn_arg_tys = ()
    insn_vararg_ty = None
    __match_args__ = ("args",)


class InsnGetValue(InsnPure):
    """
    This can be InsnPure (instead of reading memory) because it's UB to mutate
    a cons list used in a value-list.
    """

    insn_name = "get-value"
    insn_ret_ty = "value"
    insn_arg_tys = ("value-list",)
    insn_vararg_ty = None
    __match_args__ = ("args", "i")

    i: int

    def __init__(self, value: Insn, i: int):
        self.i = i
        super().__init__(value)

    def __repr__(self):
        return f"{self.name} <- get-value {self.args[0].name}, {self.i}"


class InsnMakeValueList(InsnPure):
    insn_name = "make-value-list"
    insn_ret_ty = "value-list"
    insn_arg_tys = ()
    insn_vararg_ty = "value"
    __match_args__ = ("args",)


class InsnPhi(Insn):
    insn_name = "phi"
    insn_ret_ty = "value"
    insn_arg_tys = ()
    insn_vararg_ty = None
    __match_args__ = ("args",)

    def effects(self) -> Iterable[Effect]:
        yield Effect("phi-upsilon", "r")

    def new_upsilon(self, value: Insn) -> "InsnUpsilon":
        return InsnUpsilon(value, self)


class InsnPtrRead(Insn):
    insn_name = "ptr-read"
    insn_ret_ty = "value"
    insn_arg_tys = ("value-ptr",)
    insn_vararg_ty = None
    __match_args__ = ("args",)

    def effects(self) -> Iterable[Effect]:
        yield Effect("memory", "r")


class InsnPtrWrite(Insn):
    insn_name = "ptr-write"
    insn_ret_ty = "unit"
    insn_arg_tys = ("value-ptr", "value")
    insn_vararg_ty = None
    __match_args__ = ("args",)

    def effects(self) -> Iterable[Effect]:
        yield Effect("memory", "w")


class InsnRet(Insn):
    insn_name = "ret"
    insn_ret_ty = "never"
    insn_arg_tys = ("value-list",)
    insn_vararg_ty = None
    __match_args__ = ("args",)

    def effects(self) -> Iterable[Effect]:
        yield Effect("control", "w")


class InsnSymbolFunction(Insn):
    insn_name = "symbol-function"
    insn_ret_ty = "value"
    insn_arg_tys = ("value",)
    insn_vararg_ty = None
    __match_args__ = ("args",)

    def effects(self) -> Iterable[Effect]:
        yield Effect("symbol-function", "r")


class InsnTailCall(Insn):
    insn_name = "tailcall"
    insn_ret_ty = "never"
    insn_arg_tys = ("value", "value-list")
    insn_vararg_ty = None
    __match_args__ = ("args",)

    def effects(self) -> Iterable[Effect]:
        yield Effect("control", "w")


class InsnUnreachable(Insn):
    insn_name = "unreachable"
    insn_ret_ty = "never"
    insn_arg_tys = ()
    insn_vararg_ty = None
    __match_args__ = ("args",)

    def effects(self) -> Iterable[Effect]:
        yield Effect("control", "w")


class InsnUpsilon(Insn):
    insn_name = "upsilon"
    insn_ret_ty = "unit"
    insn_arg_tys = ("value",)
    insn_vararg_ty = None
    __match_args__ = ("args", "shadow")

    shadow: InsnPhi

    def __init__(self, value: Insn, shadow: Insn):
        assert isinstance(shadow, InsnPhi)
        self.shadow = shadow
        super().__init__(value)

    def __repr__(self):
        return f"{self.name} <- upsilon {self.args[0]}, ^{self.shadow}"

    def effects(self) -> Iterable[Effect]:
        yield Effect("phi-upsilon", "w")


class InsnValueListLength(InsnPure):
    insn_name = "value-list-length"
    insn_ret_ty = "value"
    insn_arg_tys = ("value-list",)
    insn_vararg_ty = None
    __match_args__ = ("args",)
