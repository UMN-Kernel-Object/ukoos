# SPDX-FileCopyrightText: 2026 ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from dataclasses import dataclass
from typing import Self

"""
The basic object model.
"""

### Define the Python classes and types used to represent Z values.


@dataclass
class ZClassedObj:
    klass: "ZVal"

    def get_slot(self, slot: "ZSymbol") -> "ZVal":
        raise Exception(f"cannot get slot {slot!r} of {self!r}")

    def set_slot(self, slot: "ZSymbol", value: "ZVal") -> Self:
        raise Exception(f"cannot set slot {slot!r} of {self!r} to {value!r}")


@dataclass
class ZSlottedObj(ZClassedObj):
    slots: dict["ZSymbol", "ZVal"]

    def __init__(self, klass: "ZVal"):
        super().__init__(klass)
        self.slots = {}

    def get_slot(self, slot: "ZSymbol") -> "ZVal":
        return self.slots[slot]

    def set_slot(self, slot: "ZSymbol", value: "ZVal"):
        self.slots[slot] = value
        return self


@dataclass
class ZSimpleArrayT:
    elems: tuple["ZVal", ...]


@dataclass
class ZStructObj:
    klass: "ZVal"
    slots: tuple["ZVal", ...]


ZVal = (
    int
    | str
    | tuple[()]
    | tuple["ZVal", "ZVal"]
    | ZClassedObj
    | ZSimpleArrayT
    | ZStructObj
)
ZNil: ZVal = ()

### Define some builtin objects that need extra data associated with them that's not in slots.


@dataclass
class PackageSymbol:
    symbol: "ZSymbol"
    exported: bool = False


@dataclass
class ZPackage(ZSlottedObj):
    symbols: dict[str, PackageSymbol]

    def __init__(self, name: str):
        super().__init__(ClassPackage)
        self.name = name
        self.symbols = {}

    def get_slot(self, slot: "ZSymbol") -> "ZVal":
        if slot is SymbolPackageName:
            return self.name
        else:
            return super().get_slot(slot)

    def set_slot(self, slot: "ZSymbol", value: "ZVal"):
        if slot is SymbolPackageName:
            assert isinstance(value, str)
            self.name = value
            return self
        else:
            return super().set_slot(slot, value)

    def intern(self, name: str, *, exported=True) -> "ZSymbol":
        if name in self.symbols:
            return self.symbols[name].symbol
        else:
            out = ZSymbol(name, self)
            self.symbols[name] = PackageSymbol(out, exported)
            return out


@dataclass
class ZSymbol(ZSlottedObj):
    def __init__(self, name: str, pkg: ZPackage):
        super().__init__(ClassSymbol)
        assert name not in pkg.symbols
        self.name = name
        self.pkg = pkg

    def get_slot(self, slot: "ZSymbol") -> "ZVal":
        if slot is SymbolSymbolName:
            return self.name
        else:
            return super().get_slot(slot)

    def set_slot(self, slot: "ZSymbol", value: "ZVal"):
        if slot is SymbolSymbolName:
            assert isinstance(value, str)
            self.name = value
            return self
        else:
            return super().set_slot(slot, value)

    def __eq__(self, other):
        return self is other

    def __hash__(self) -> int:
        return hash((self.pkg.name, self.name))

    def __repr__(self) -> str:
        return f"{self.pkg.name}::{self.name}"


### Define helpers for making Z lists and simple vectors.


def make_list(*elems: ZVal) -> tuple[()] | tuple[ZVal, ZVal]:
    out = ()
    for elem in reversed(elems):
        out = (elem, out)
    return out


def make_vec(*elems: ZVal) -> ZSimpleArrayT:
    return ZSimpleArrayT(elems=elems)


### Bootstrap the root of the class hierarchy.
###
### Naming convention: classes have names of the form ClassFoo, packages have
### names of the form PackageFoo, symbols in Z have names of the form SymbolFoo.

## Define Z:BUILTIN-CLASS, but don't define its slots.
ClassBuiltinClass = ZSlottedObj(ZNil)
ClassBuiltinClass.klass = ClassBuiltinClass

## Define objects we need to make symbols.
ClassPackage = ZSlottedObj(ClassBuiltinClass)
ClassSymbol = ZSlottedObj(ClassBuiltinClass)
PackageZ = ZPackage("Z")
SymbolPackageName = PackageZ.intern("PACKAGE/NAME")
SymbolSymbolName = PackageZ.intern("SYMBOL/NAME")
SymbolSymbolPackage = PackageZ.intern("SYMBOL/PACKAGE")

## Define objects we need to define slots.
ClassDirectSlotDefinition = ZSlottedObj(ClassBuiltinClass)
ClassEffectiveSlotDefinition = ZSlottedObj(ClassBuiltinClass)

## Define the names of the slots of class objects.
SymbolClassName = PackageZ.intern("CLASS/NAME")
SymbolClassFinalized = PackageZ.intern("CLASS/FINALIZED?")
SymbolClassDirectSlots = PackageZ.intern("CLASS/DIRECT-SLOTS")
SymbolClassDirectSuperclasses = PackageZ.intern("CLASS/DIRECT-SUPERCLASSES")
SymbolClassDirectSubclasses = PackageZ.intern("CLASS/DIRECT-SUBCLASSES")
SymbolClassEffectiveSlots = PackageZ.intern("CLASS/EFFECTIVE-SLOTS")
SymbolClassEffectiveSuperclasses = PackageZ.intern("CLASS/EFFECTIVE-SUPERCLASSES")
SymbolClassEffectiveSubclasses = PackageZ.intern("CLASS/EFFECTIVE-SUBCLASSES")


## Set the direct slots of the classes we've defined so far.


def make_direct_slot_definition():
    # TODO
    pass


ClassBuiltinClass.set_slot(SymbolClassName, PackageZ.intern("BUILTIN-CLASS"))
ClassBuiltinClass.set_slot(SymbolClassFinalized, ZNil)
ClassBuiltinClass.set_slot(SymbolClassDirectSlots, make_vec())
ClassBuiltinClass.set_slot(SymbolClassDirectSuperclasses, make_vec())
ClassBuiltinClass.set_slot(SymbolClassDirectSubclasses, make_vec())

ClassDirectSlotDefinition.set_slot(
    SymbolClassName, PackageZ.intern("DIRECT-SLOT-DEFINITION")
)
ClassDirectSlotDefinition.set_slot(SymbolClassFinalized, ZNil)
ClassDirectSlotDefinition.set_slot(SymbolClassDirectSlots, make_vec())
ClassDirectSlotDefinition.set_slot(SymbolClassDirectSuperclasses, make_vec())
ClassDirectSlotDefinition.set_slot(SymbolClassDirectSubclasses, make_vec())

ClassEffectiveSlotDefinition.set_slot(
    SymbolClassName, PackageZ.intern("EFFECTIVE-SLOT-DEFINITION")
)
ClassEffectiveSlotDefinition.set_slot(SymbolClassFinalized, ZNil)
ClassEffectiveSlotDefinition.set_slot(SymbolClassDirectSlots, make_vec())
ClassEffectiveSlotDefinition.set_slot(SymbolClassDirectSuperclasses, make_vec())
ClassEffectiveSlotDefinition.set_slot(SymbolClassDirectSubclasses, make_vec())

ClassPackage.set_slot(SymbolClassName, PackageZ.intern("PACKAGE"))
ClassPackage.set_slot(SymbolClassFinalized, ZNil)
ClassPackage.set_slot(SymbolClassDirectSlots, make_vec())  # TODO
ClassPackage.set_slot(SymbolClassDirectSuperclasses, make_vec())
ClassPackage.set_slot(SymbolClassDirectSubclasses, make_vec())

ClassSymbol.set_slot(SymbolClassName, PackageZ.intern("SYMBOL"))
ClassSymbol.set_slot(SymbolClassFinalized, ZNil)
ClassSymbol.set_slot(SymbolClassDirectSlots, make_vec())  # TODO
ClassSymbol.set_slot(SymbolClassDirectSuperclasses, make_vec())
ClassSymbol.set_slot(SymbolClassDirectSubclasses, make_vec())

## TODO
SymbolClass = PackageZ.intern("CLASS")
SymbolPackageName = PackageZ.intern("PACKAGE/NAME")
SymbolString = PackageZ.intern("STRING")

### Define some other primitive classes.

# ClassClass = ZObj(ClassBuiltinClass)
# ClassString = ZObj(ClassBuiltinClass)
# ClassCons = ZObj(ClassBuiltinClass)
# ClassInt = ZObj(ClassBuiltinClass)
# ClassNull = ZObj(ClassBuiltinClass)

### Bootstrap Z:CLASS-OF.
###
### Naming convention: built-in functions' implementations have snake_case
### names, their function objects have names of the form FunctionFoo.


def class_of(value: ZVal) -> ZVal:
    if isinstance(value, int):
        return ClassInt
    elif isinstance(value, str):
        return ClassString
    elif isinstance(value, tuple):
        if len(value) == 0:
            return ClassNull
        elif len(value) == 2:
            return ClassCons
    elif isinstance(value, ZClassedObj):
        return value.klass
    elif isinstance(value, ZSimpleArrayT):
        raise Exception("TODO")
    elif isinstance(value, ZStructObj):
        return value.klass
