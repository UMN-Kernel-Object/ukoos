# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from dataclasses import dataclass, field
from typing import Literal, overload

"""
The basic object model.
"""

### Define the Python classes and types used to represent Z values.


class ZStr:
    buf: bytes
    off: int
    len: int

    @overload
    def __init__(self, buf: str): ...

    @overload
    def __init__(self, buf: bytes, off: int, len: int, /): ...

    def __init__(self, buf, *args):
        if isinstance(buf, str):
            assert len(args) == 0
            self.buf = buf.encode("utf-8")
            self.off = 0
            self.len = len(buf)
        else:
            assert isinstance(buf, bytes)
            assert len(args) == 2
            assert isinstance(args[0], int)
            assert isinstance(args[1], int)
            assert args[0] + args[1] <= len(buf)
            self.buf = buf
            self.off = args[0]
            self.len = args[1]

    def __eq__(self, other) -> bool:
        if not isinstance(other, ZStr):
            return False
        return str(self) == str(other)

    def __getitem__(self, i: "slice[int | None, int | None, None]") -> "ZStr":
        start = i.start
        stop = i.stop
        if start is None:
            start = 0
        if stop is None:
            stop = self.len

        assert start <= stop
        assert stop <= self.len
        return ZStr(self.buf, self.off + start, stop - start)

    def __hash__(self) -> int:
        return hash(str(self))

    def __len__(self) -> int:
        return self.len

    def __repr__(self) -> str:
        return f"ZStr({str(self)!r})"

    def __str__(self) -> str:
        return self.buf[self.off : self.off + self.len].decode("utf-8")

    def decode_one_utf8(self, i: int = 0) -> tuple[int, int]:
        """
        Decodes a rune at the given offset, returning it and its length.
        """

        if i != 0:
            self = self[i:]
        first_byte = self.buf[self.off]
        if 0 <= first_byte < 0x80:
            return (first_byte, 1)
        elif 0xC0 <= first_byte < 0xE0:
            n = (first_byte & 0x1F) << 6
            n |= self.buf[self.off + 1] & 0x3F
            return (n, 2)
        elif 0xE0 <= first_byte < 0xF0:
            n = (first_byte & 0x1F) << 12
            n |= (self.buf[self.off + 1] & 0x3F) << 6
            n |= self.buf[self.off + 2] & 0x3F
            return (n, 3)
        elif 0xF0 <= first_byte < 0xF8:
            n = (first_byte & 0x1F) << 18
            n |= (self.buf[self.off + 1] & 0x3F) << 12
            n |= (self.buf[self.off + 2] & 0x3F) << 6
            n |= self.buf[self.off + 3] & 0x3F
            return (n, 4)
        else:
            raise Exception("ZStr.decode_one_utf8: invalid UTF-8?")


ALL_MODS: dict[ZStr, "ZMod"] = {}

ZSymStatus = Literal["INTERNAL", "EXTERNAL", "INHERITED"]


@dataclass
class ZMod:
    name: ZStr
    syms: dict[ZStr, tuple["ZSym", ZSymStatus]] = field(
        default_factory=dict, init=False
    )

    @staticmethod
    def find(name: str | ZStr) -> "ZMod":
        if not isinstance(name, ZStr):
            name = ZStr(name)
        return ALL_MODS[name]

    @staticmethod
    def make(name: ZStr) -> "ZMod":
        assert name not in ALL_MODS
        out = ZMod(name)
        ALL_MODS[name] = out
        return out

    def intern(self, name: str | ZStr) -> tuple["ZSym", ZSymStatus]:
        if not isinstance(name, ZStr):
            name = ZStr(name)
        if name in self.syms:
            return self.syms[name]
        else:
            status = "INTERNAL"
            if self.name == ZStr("KEYWORD"):
                status = "EXTERNAL"
            out = (ZSym(self, name), status)
            self.syms[name] = out
            return out


ZMod.make(ZStr("%Z"))
ZMod.make(ZStr("KEYWORD"))
ZMod.make(ZStr("Z"))


@dataclass
class ZSym:
    mod: ZMod
    name: ZStr

    function: "ZSym | None" = field(default=None, init=False)
    macro: "ZSym | None" = field(default=None, init=False)
    setf: "ZSym | None" = field(default=None, init=False)
    value: "ZSym | None" = field(default=None, init=False)

    @staticmethod
    def keyword(name: str) -> "ZSym":
        return ZMod.find("KEYWORD").intern(name)[0]

    @staticmethod
    def impl(name: str) -> "ZSym":
        return ZMod.find("%Z").intern(name)[0]

    @staticmethod
    def z(name: str) -> "ZSym":
        return ZMod.find("Z").intern(name)[0]

    def __eq__(self, other) -> bool:
        if not isinstance(other, ZSym):
            return False
        return self is other

    def __hash__(self) -> int:
        return hash(id(self))

    def __repr__(self) -> str:
        if self.is_keyword:
            return f":{self.name}"
        elif self.mod.name == ZStr("Z") and self.name == ZStr("NIL"):
            return "()"
        elif self.mod.syms[self.name][1] == "EXTERNAL":
            return f"{self.mod.name}:{self.name}"
        else:
            return f"{self.mod.name}::{self.name}"

    @property
    def is_keyword(self) -> bool:
        return self.mod.name == ZStr("KEYWORD")

    def is_the_keyword(self, name: str | ZStr) -> bool:
        if not isinstance(name, ZStr):
            name = ZStr(name)
        assert self.mod.name == ZStr("KEYWORD")
        return self.name == name


NIL = ZSym.z("NIL")
T = ZSym.z("T")


@dataclass
class ZCons:
    car: "ZVal"
    cdr: "ZVal"

    @staticmethod
    def of_list(*args: "ZVal") -> "ZVal":
        out = NIL
        for arg in reversed(args):
            out = ZCons(arg, out)
        return out

    def __repr__(self) -> str:
        out = ""
        ch = "("
        while isinstance(self, ZCons):
            out += ch
            out += repr(self.car)
            ch = " "
            self = self.cdr
        if self is not NIL:
            out += " . "
            out += repr(self)
        out += ")"
        return out

    def collect(self: "ZVal") -> list["ZVal"]:
        out = []
        while self is not NIL:
            assert isinstance(self, ZCons)
            out.append(self.car)
            self = self.cdr
        return out


ZVal = float | int | ZCons | ZStr | ZSym
