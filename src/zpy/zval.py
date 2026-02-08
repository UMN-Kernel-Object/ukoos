# SPDX-FileCopyrightText: 2026 ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from dataclasses import dataclass
from typing import Optional, Self

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
Nil: ZVal = ()

### Define some builtin objects that need extra data associated with them that's not in slots.


@dataclass
class ModuleSymbol:
    symbol: "ZSymbol"
    exported: bool = False


@dataclass
class ZModule(ZSlottedObj):
    symbols: dict[str, ModuleSymbol]

    def __init__(self, name: str):
        super().__init__(ClassModule)
        self.name = name
        self.symbols = {}

    def get_slot(self, slot: "ZSymbol") -> "ZVal":
        if slot is SymbolModuleName:
            return self.name
        else:
            return super().get_slot(slot)

    def set_slot(self, slot: "ZSymbol", value: "ZVal"):
        if slot is SymbolModuleName:
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
            self.symbols[name] = ModuleSymbol(out, exported)
            return out


@dataclass
class ZSymbol(ZSlottedObj):
    def __init__(self, name: str, mod: ZModule):
        super().__init__(ClassSymbol)
        assert name not in mod.symbols
        self.name = name
        self.mod = mod

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
        return hash((self.mod.name, self.name))

    def __repr__(self) -> str:
        return f"{self.mod.name}::{self.name}"


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
### Naming convention: classes have names of the form ClassFoo, modules have
### names of the form ModuleFoo, symbols in Z have names of the form SymbolFoo.

## Define Z:BUILTIN-CLASS, but don't define its slots.
ClassBuiltInClass = ZSlottedObj(Nil)
ClassBuiltInClass.klass = ClassBuiltInClass

## Define objects we need to make symbols.
ClassModule = ZSlottedObj(ClassBuiltInClass)
ClassSymbol = ZSlottedObj(ClassBuiltInClass)
ModuleZ = ZModule("Z")
SymbolModuleName = ModuleZ.intern("MODULE/NAME")
SymbolSymbolName = ModuleZ.intern("SYMBOL/NAME")
SymbolSymbolModule = ModuleZ.intern("SYMBOL/MODULE")

## Define T.
T = ModuleZ.intern("T")

## Define slot definitions.
ClassStandardDirectSlotDefinition = ZSlottedObj(ClassBuiltInClass)


def make_direct_slot_definition(
    name: str,
    *,
    initform: Optional[ZVal] = None,
    initargs: list[ZVal] = [],
    reader: Optional[str] = None,
    writer: Optional[str] = None,
    # allocation: Optional[str] = None,
) -> ZVal:
    slot = ZSlottedObj(ClassStandardDirectSlotDefinition)
    slot.set_slot(ModuleZ.intern("SLOT-DEFINITION/NAME"), ModuleZ.intern(name))
    if initform is not None:
        slot.set_slot(ModuleZ.intern("SLOT-DEFINITION/INITFORM"), initform)
        # TODO: initfunction
    slot.set_slot(ModuleZ.intern("SLOT-DEFINITION/INITARGS"), make_list(*initargs))
    slot.set_slot(
        ModuleZ.intern("SLOT-DEFINITION/READERS"),
        make_list(ModuleZ.intern(reader) if reader is not None else Nil),
    )
    slot.set_slot(
        ModuleZ.intern("SLOT-DEFINITION/WRITERS"),
        make_list(ModuleZ.intern(writer) if writer is not None else Nil),
    )
    # slot.set_slot(ModuleZ.intern("ALLOCATION"), ModuleZ.intern(name))
    # slot.set_slot(ModuleZ.intern("ALLOCATION-CLASS"), ModuleZ.intern(name))
    # TODO
    return slot


## Define helpers for defining classes.


def add_direct_subclass(child: ZSlottedObj, parent: ZSlottedObj):
    SymbolClassDirectSuperclasses = ModuleZ.intern("CLASS/DIRECT-SUPERCLASSES")
    SymbolClassDirectSubclasses = ModuleZ.intern("CLASS/DIRECT-SUBCLASSES")
    child.set_slot(
        SymbolClassDirectSuperclasses,
        (parent, child.get_slot(SymbolClassDirectSubclasses)),
    )
    parent.set_slot(
        SymbolClassDirectSubclasses,
        (child, parent.get_slot(SymbolClassDirectSubclasses)),
    )


def init_class(
    klass: ZSlottedObj,
    name: str,
    *direct_slots: ZVal,
    direct_superclasses: list[ZSlottedObj] = [],
):
    klass.set_slot(ModuleZ.intern("CLASS/NAME"), ModuleZ.intern(name))
    klass.set_slot(ModuleZ.intern("CLASS/FINALIZED?"), Nil)
    klass.set_slot(ModuleZ.intern("CLASS/DIRECT-SLOTS"), make_list(*direct_slots))
    klass.set_slot(ModuleZ.intern("CLASS/DIRECT-SUPERCLASSES"), Nil)
    klass.set_slot(ModuleZ.intern("CLASS/DIRECT-SUBCLASSES"), Nil)
    for parent in direct_superclasses:
        add_direct_subclass(klass, parent)


def defclass(
    name: str,
    *direct_slots: ZVal,
    sup: Optional[list[ZSlottedObj]] = None,
    metaclass: Optional[ZVal] = None,
) -> ZSlottedObj:
    if sup is None:
        sup = [ClassStandardObject]
    if metaclass is None:
        metaclass = ClassStandardClass
    klass = ZSlottedObj(metaclass)
    init_class(klass, name, *direct_slots, direct_superclasses=sup)
    return klass


### Define the rest of the metaobjects and fill out the fields on the classes
### we've already defined.

ClassStandardClass = defclass("STANDARD-CLASS", sup=[], metaclass=Nil)
ClassStandardClass.klass = ClassStandardClass

ClassT = defclass("T", sup=[], metaclass=ClassBuiltInClass)
ClassStandardObject = defclass("STANDARD-OBJECT", sup=[ClassT])

ClassSpecializer = defclass("SPECIALIZER")
ClassEqlSpecializer = defclass(
    "EQL-SPECIALIZER",
    make_direct_slot_definition("EQL-SPECIALIZER/OBJECT"),
    make_direct_slot_definition("EQL-SPECIALIZER/DIRECT-METHODS"),
    sup=[ClassSpecializer],
)
ClassClass = defclass(
    "CLASS",
    make_direct_slot_definition("CLASS/NAME"),
    make_direct_slot_definition("CLASS/CLASS-EQ-SPECIALIZER"),
    make_direct_slot_definition("CLASS/DIRECT-SUPERCLASSES"),
    make_direct_slot_definition("CLASS/DIRECT-SUBCLASSES"),
    make_direct_slot_definition("CLASS/DIRECT-METHODS"),
    make_direct_slot_definition("CLASS/FINALIZED-P"),
    sup=[ClassSpecializer],
)
init_class(ClassBuiltInClass, "BUILT-IN-CLASS", direct_superclasses=[ClassClass])
add_direct_subclass(ClassStandardClass, ClassClass)
ClassForwardReferencedClass = defclass("FORWARD-REFERENCED-CLASS")
ClassFuncallableStandardClass = defclass("FUNCALLABLE-STANDARD-CLASS", sup=[ClassClass])

ClassSlotDefinition = defclass(
    "SLOT-DEFINITION",
    make_direct_slot_definition("SLOT-DEFINITION/NAME"),
    make_direct_slot_definition("SLOT-DEFINITION/INITFORM"),
    make_direct_slot_definition("SLOT-DEFINITION/INITFUNCTION"),
    make_direct_slot_definition("SLOT-DEFINITION/INITARGS"),
)
ClassDirectSlotDefinition = defclass(
    "DIRECT-SLOT-DEFINITION",
    make_direct_slot_definition("DIRECT-SLOT-DEFINITION/READERS"),
    make_direct_slot_definition("DIRECT-SLOT-DEFINITION/WRITERS"),
    sup=[ClassSlotDefinition],
)
ClassEffectiveSlotDefinition = defclass(
    "EFFECTIVE-SLOT-DEFINITION", sup=[ClassSlotDefinition]
)
ClassStandardSlotDefinition = defclass(
    "STANDARD-SLOT-DEFINITION", sup=[ClassSlotDefinition]
)
init_class(
    ClassStandardDirectSlotDefinition,
    "STANDARD-DIRECT-SLOT-DEFINITION",
    direct_superclasses=[ClassStandardSlotDefinition, ClassDirectSlotDefinition],
)
ClassStandardEffectiveSlotDefinition = defclass(
    "STANDARD-EFFECTIVE-SLOT-DEFINITION",
    make_direct_slot_definition("STANDARD-EFFECTIVE-SLOT-DEFINITION/LOCATION"),
    sup=[ClassStandardSlotDefinition, ClassEffectiveSlotDefinition],
)

ClassMethodCombination = defclass("METHOD-COMBINATION")

ClassMethod = defclass("METHOD")
ClassStandardMethod = defclass(
    "STANDARD-METHOD",
    make_direct_slot_definition("STANDARD-METHOD/QUALIFIERS"),
    make_direct_slot_definition("STANDARD-METHOD/SPECIALIZERS"),
    make_direct_slot_definition("STANDARD-METHOD/LAMBDA-LIST"),
    sup=[ClassMethod],
)
ClassStandardAccessorMethod = defclass(
    "STANDARD-ACCESSOR-METHOD", sup=[ClassStandardMethod]
)
ClassStandardReaderMethod = defclass(
    "STANDARD-READER-METHOD", sup=[ClassStandardAccessorMethod]
)
ClassStandardWriterMethod = defclass(
    "STANDARD-WRITER-METHOD", sup=[ClassStandardAccessorMethod]
)

ClassFuncallableStandardObject = defclass("FUNCALLABLE-STANDARD-OBJECT")
ClassGenericFunction = defclass(
    "GENERIC-FUNCTION", sup=[ClassFuncallableStandardObject]
)
ClassStandardGenericFunction = defclass(
    "STANDARD-GENERIC-FUNCTION",
    make_direct_slot_definition("STANDARD-GENERIC-FUNCTION/NAME"),
    make_direct_slot_definition("STANDARD-GENERIC-FUNCTION/METHODS"),
    make_direct_slot_definition("STANDARD-GENERIC-FUNCTION/METHOD-CLASS"),
    make_direct_slot_definition("STANDARD-GENERIC-FUNCTION/METHOD-COMBINATION"),
    make_direct_slot_definition("STANDARD-GENERIC-FUNCTION/DECLARATIONS"),
    sup=[ClassGenericFunction],
)

### Implement class finalization and finalize the metaobject classes.


def finalize_inheritance(klass: ZSlottedObj):
    pass


### Define some other primitive classes.

# ClassClass = ZObj(ClassBuiltInClass)
# ClassString = ZObj(ClassBuiltInClass)
# ClassCons = ZObj(ClassBuiltInClass)
# ClassInt = ZObj(ClassBuiltInClass)
# ClassNull = ZObj(ClassBuiltInClass)

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
