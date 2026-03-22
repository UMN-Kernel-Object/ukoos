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
from typing import Iterable, Iterator, Literal
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
    parent: tuple["Block", int] | None

    def __init__(self, *args: "Insn"):
        assert type(self) is not Insn
        self.name = default_insn_name()
        self.args = list(args)
        self.parent = None
        self.tyck()

    @abstractmethod
    def effects(self) -> Iterable[Effect]: ...

    def dominates(self, other: "Insn") -> bool:
        """
        Returns whether self strictly dominates other.
        """

        assert self.parent is not None
        assert other.parent is not None
        if self.parent[0] is other.parent[0]:
            return self.parent[1] < other.parent[1]
        else:
            return self.parent[0].dominates(other.parent[0])

    @property
    def func(self) -> "Func":
        assert self.parent is not None
        assert self.parent[0].parent is not None
        return self.parent[0].parent[0]

    @property
    def jumps_to(self) -> tuple["Block", ...] | None:
        return None

    def tyck(self):
        arg_tys, vararg_ty = self.type_args

        if vararg_ty is None:
            assert len(self.args) == len(arg_tys)
        else:
            assert len(self.args) >= len(arg_tys)
            for arg in self.args[len(arg_tys) :]:
                assert arg.type == vararg_ty

        for arg, ty in zip(self.args, arg_tys):
            assert arg.type == ty

        if self.parent is not None and self.parent[0].parent is not None:
            for arg in self.args:
                assert self.func is arg.func
                assert arg.dominates(self)

    @property
    def type(self) -> Type:
        return type(self).insn_ret_ty

    @property
    def type_args(self) -> tuple[tuple[Type, ...], Type | None]:
        cls = type(self)
        return (cls.insn_arg_tys, cls.insn_vararg_ty)

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
    _insns: list[Insn]
    parent: tuple["Func", int] | None

    def __init__(self, parent: tuple["Func", int], *, name: str | None = None):
        if name is None:
            name = default_block_name()
        self.name = name
        self._insns = []
        self.parent = parent

    def __getitem__(self, i: int) -> Insn:
        return self._insns[i]

    def __iter__(self) -> Iterator[Insn]:
        return iter(self._insns)

    def __len__(self) -> int:
        return len(self._insns)

    def add[T: Insn](self, insn: T) -> T:
        insn.parent = (self, len(self))
        self._insns.append(insn)
        if self.parent is not None:
            self.parent[0]._dom = None
            self.parent[0]._preds = None
        return insn

    def dominates(self, other: "Block") -> bool:
        """
        Returns whether self strictly dominates other.
        """

        assert self.parent is not None
        assert other.parent is not None
        assert self.parent[0] is other.parent[0]

        func = self.parent[0]
        if self is other:
            return False

        while True:
            other = func.dom.idom(other)
            if self is other:
                return True
            if other is func.entry:
                return False

    @property
    def jumps_to(self) -> tuple["Block", ...]:
        """
        Returns the blocks this block can jump to (i.e., its successors).
        """

        for insn in self:
            jumps_to = insn.jumps_to
            if jumps_to is not None:
                return jumps_to
        raise Exception(f"Block {self} had no terminator")

    def remove_all(self, indices: list[int]) -> list[Insn]:
        for i in indices:
            assert 0 <= i < len(self)
        to_remove = frozenset(indices)
        removed: dict[int, Insn] = {}
        new_insns = []
        for i, insn in enumerate(self):
            insn.parent = None
            if i in to_remove:
                removed[i] = insn
            else:
                new_insns.append(insn)

        self._insns.clear()
        for insn in new_insns:
            insn.parent = (self, len(self))
            self._insns.append(insn)
        if self.parent is not None:
            self.parent[0].invalidate()

        return [removed[i] for i in indices]

    def tyck(self):
        for insn in self:
            insn.tyck()

    def __repr__(self) -> str:
        out = f"{self.name}:"
        for insn in self:
            out += f"\n\t{insn!r}"
        return out


@dataclass(init=False)
class DomTree:
    """
    A dominator tree. This is a tree where the parent-child relation is the
    "immediately dominates" relation.
    """

    func: "Func"
    _idom: list[int]
    children: list[list[int]]

    def __init__(self, func: "Func", idom: list[int]):
        self.func = func
        self._idom = idom
        self.children = [
            [j for j in range(len(idom)) if idom[j] == i and j != 0]
            for i in range(len(idom))
        ]

    def idom(self, block: Block) -> Block:
        assert block.parent is not None
        assert block.parent[0] is self.func
        return self.func[self._idom[block.parent[1]]]

    def preorder(self) -> Iterator[Block]:
        def traverse(i: int):
            yield self.func[i]
            for child in self.children[i]:
                yield from traverse(child)

        yield from traverse(0)

    def postorder(self) -> Iterator[Block]:
        def traverse(i: int):
            for child in self.children[i]:
                yield from traverse(child)
            yield self.func[i]

        yield from traverse(0)


@dataclass(init=False)
class Func:
    name: ZSym
    args: Insn
    effect_registry: EffectRegistry
    _blocks: list[Block]
    _dom: DomTree | None
    _preds: list[set[int]] | None

    def __init__(self, name: ZSym = NIL):
        self.name = name
        self.effect_registry = EffectRegistry()
        self._blocks = []
        self._dom = None
        self._preds = None

        entry_block = self.new_block()
        self.args = entry_block.add(InsnGetArgs())

    def __getitem__(self, i: int) -> Block:
        return self._blocks[i]

    def __iter__(self) -> Iterator[Block]:
        return iter(self._blocks)

    def __len__(self) -> int:
        return len(self._blocks)

    def __repr__(self) -> str:
        out = f"func ({self.name} {self.args.name}) {{\n"
        for block in self:
            out += f"{block!r}\n"
        out += "}"
        return out

    def _compute_dom(self) -> DomTree:
        """
        Computes the dominator tree.
        """

        # Reorder to be in reverse post-order. This lets intersect take
        # advantage of indices being in this order.
        self.reorder_blocks()
        if self._preds is None:
            self._preds = self._compute_preds()
        preds = self._preds
        assert preds is not None

        doms: list[int | None] = [None] * len(self)
        doms[0] = 0

        def intersect(i: int, j: int) -> int:
            """
            Logically, intersect computes the intersection of the dominance
            sets. Thanks to the other properties established by this algorithm,
            it "really" just finds the nearest common dominator.

            Because of the reverse post-ordering established above, we have the
            invariant "for x!=0, doms[x] < x." This makes sense: any dominator
            must be a predecessor.

            We take turns walking up the dominator tree until we find a common
            ancestor. Because it's an ancestor of each, it must dominate both.
            """

            while i != j:
                while i > j:
                    di = doms[i]
                    assert di is not None
                    i = di
                while j > i:
                    dj = doms[j]
                    assert dj is not None
                    j = dj
            return i

        # We run to a fixed point. In each step, we might "discover" another
        # path from the root to a block, and so have to change its expected
        # immediate dominator to be further up the tree.
        changed = True
        while changed:
            changed = False
            for i in range(1, len(self)):
                index = None
                for p in preds[i]:
                    if doms[p] is not None:
                        if index is None:
                            index = p
                        else:
                            index = intersect(index, p)
                # For us to have reached a block, we must have reached at least
                # one predecessor.
                assert index is not None
                if doms[i] != index:
                    doms[i] = index
                    changed = True

        def ensure(i: int | None) -> int:
            assert i is not None
            return i

        return DomTree(self, [ensure(i) for i in doms])

    def _compute_preds(self) -> list[set[int]]:
        """
        Computes the predecessors of the block.
        """

        preds: list[set[int]] = [set() for _ in range(len(self))]
        for block in self:
            assert block.parent is not None
            assert block.parent[0] is self
            for succ in block.jumps_to:
                assert succ.parent is not None
                assert succ.parent[0] is self
                preds[succ.parent[1]].add(block.parent[1])
        return preds

    @property
    def dom(self) -> DomTree:
        if self._dom is None:
            self._dom = self._compute_dom()
        return self._dom

    @property
    def entry(self) -> Block:
        return self[0]

    def invalidate(self):
        """
        Marks analyses stored on the function itself that depend on the blocks
        as invalid.
        """

        self._dom = None
        self._preds = None

    def new_block(self, *, name: str | None = None) -> Block:
        block = Block(parent=(self, len(self._blocks)), name=name)
        self._blocks.append(block)
        self.invalidate()
        return block

    def preds(self, block: Block) -> Iterator[Block]:
        if self._preds is None:
            self._preds = self._compute_preds()
        assert block.parent is not None
        assert block.parent[0] is self
        return (self[i] for i in self._preds[block.parent[1]])

    def reorder_blocks(self):
        new_blocks: list[Block] = []
        seen = set()

        def traverse(block: Block):
            assert block.parent is not None
            assert block.parent[0] is self
            if block.parent[1] in seen:
                return
            seen.add(block.parent[1])

            for child in reversed(block.jumps_to):
                traverse(child)
            new_blocks.append(block)

        entry = self.entry
        traverse(entry)

        self._blocks.clear()
        for block in self:
            block.parent = None
        for i, block in enumerate(reversed(new_blocks)):
            block.parent = (self, i)
            self._blocks.append(block)
        self.invalidate()

        assert self.entry is entry

    def tyck(self):
        for block in self:
            block.tyck()


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

    @property
    def jumps_to(self) -> tuple[Block, ...] | None:
        return (self.if_t, self.if_f)


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


class InsnGoto(Insn):
    insn_name = "goto"
    insn_ret_ty = "never"
    insn_arg_tys = ()
    insn_vararg_ty = None
    __match_args__ = ("args", "dst")

    dst: Block

    def __init__(self, dst: Block):
        self.dst = dst
        super().__init__()

    def __repr__(self):
        return f"{self.name} <- goto {self.dst.name}"

    def effects(self) -> Iterable[Effect]:
        # No control write, because it's unconditional and therefore safe to
        # hoist across?
        return ()

    @property
    def jumps_to(self) -> tuple[Block, ...] | None:
        return (self.dst,)


class InsnMakeValueList(InsnPure):
    insn_name = "make-value-list"
    insn_ret_ty = "value-list"
    insn_arg_tys = ()
    insn_vararg_ty = "value"
    __match_args__ = ("args",)


class InsnPhi(Insn):
    insn_name = "phi"
    insn_arg_tys = ()
    insn_vararg_ty = None
    __match_args__ = ("type", "args")

    _type: Type

    def __init__(self, ty: Type):
        self._type = ty
        super().__init__()

    def __repr__(self):
        return f"{self.name} <- phi {self.type}"

    def effects(self) -> Iterable[Effect]:
        yield Effect("phi-upsilon", "r")

    @property
    def type(self) -> Type:
        return self._type


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

    @property
    def jumps_to(self) -> tuple[Block, ...] | None:
        return ()


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

    @property
    def jumps_to(self) -> tuple[Block, ...] | None:
        return ()


class InsnUnreachable(Insn):
    insn_name = "unreachable"
    insn_ret_ty = "never"
    insn_arg_tys = ()
    insn_vararg_ty = None
    __match_args__ = ("args",)

    def effects(self) -> Iterable[Effect]:
        yield Effect("control", "w")

    @property
    def jumps_to(self) -> tuple[Block, ...] | None:
        return ()


class InsnUpsilon(Insn):
    insn_name = "upsilon"
    insn_ret_ty = "unit"
    __match_args__ = ("args", "shadow")

    shadow: InsnPhi

    def __init__(self, value: Insn, shadow: Insn):
        assert isinstance(shadow, InsnPhi)
        self.shadow = shadow
        super().__init__(value)

    def __repr__(self):
        return f"{self.name} <- upsilon {self.args[0].name}, ^{self.shadow.name}"

    def effects(self) -> Iterable[Effect]:
        yield Effect("phi-upsilon", "w")

    @property
    def type_args(self) -> tuple[tuple[Type, ...], Type | None]:
        return ((self.shadow.type,), None)


class InsnValueListLength(InsnPure):
    insn_name = "value-list-length"
    insn_ret_ty = "value"
    insn_arg_tys = ("value-list",)
    insn_vararg_ty = None
    __match_args__ = ("args",)
