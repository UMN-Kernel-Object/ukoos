# SPDX-FileCopyrightText: 2026 ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

from dataclasses import dataclass
from inspect import getfullargspec
from typing import Callable, ClassVar, Generator, NoReturn, Protocol

"""
The basic object model.
"""

### Define the Python classes and types used to represent Z values.


class BuiltInFunction(Protocol):
    def __call__(self, *args: "ZVal") -> list["ZVal"]: ...


@dataclass
class SymbolData:
    symbol: "Symbol"
    exported: bool = False
    function: "ZVal | None" = None
    klass: "ZVal | None" = None
    macro: "ZVal | None" = None
    setf: "ZVal | None" = None
    value: "ZVal | None" = None


@dataclass
class Module:
    symbols: dict[str, SymbolData]

    ALL: ClassVar[dict[str, "Module"]] = {}

    def __init__(self, name: str):
        assert name not in Module.ALL
        self.name = name
        self.symbols = {}
        Module.ALL[name] = self

    def __getitem__(self, name: str) -> "Symbol":
        if name in self.symbols:
            return self.symbols[name].symbol
        else:
            out = Symbol(name, self)
            self.symbols[name] = SymbolData(out)
            return out


@dataclass
class ModuleKeyword(Module):
    def __init__(self):
        super().__init__("KEYWORD")

    def __getitem__(self, name: str) -> "Symbol":
        sym = super().__getitem__(name)
        sym.data.exported = True
        return sym


KEYWORD = ModuleKeyword()
Z = Module("Z")


@dataclass
class SimpleVectorT:
    elems: tuple["ZVal", ...]

    def __init__(self, *elems: "ZVal"):
        self.elems = elems

    def __str__(self) -> str:
        return "[" + " ".join(str(elem) for elem in self.elems) + "]"


@dataclass
class StandardObj:
    klass: "ZVal"
    slots: list["ZVal | None"]

    def __init__(self, klass: "ZVal"):
        (class_effective_slots,) = Z["CLASS/EFFECTIVE-SLOTS"].call(klass)
        assert isinstance(class_effective_slots, SimpleVectorT)
        self.klass = klass
        self.slots = [
            None
            for slot in class_effective_slots.elems
            if Z["SLOT-DEFINITION/ALLOCATION"].call(slot)[0] is KEYWORD["INSTANCE"]
        ]

    def __str__(self) -> str:
        if self.klass == ClassStandardClass:
            class_name = Z["STANDARD-CLASS"]
        else:
            (class_name,) = Z["CLASS/NAME"].call(self.klass)
        out = f"#<{class_name}"
        for slot in self.slots:
            out += f" {slot}"
        return out + ">"

    def slot_boundp(self, slot_name: "ZVal") -> bool:
        i = slot_location(self.klass, slot_name)
        return self.slots[i] is not None

    def slot_exists(self, slot_name: "ZVal") -> bool:
        return any(
            Z["SLOT-DEFINITION/NAME"].call(slot)[0] is slot_name
            for slot in list_iter(Z["CLASS/EFFECTIVE-SLOTS"].call(self.klass)[0])
        )

    def slot_unbind(self, slot_name: "ZVal"):
        i = slot_location(self.klass, slot_name)
        self.slots[i] = None

    def slot_value(self, slot_name: "ZVal") -> "ZVal":
        i = slot_location(self.klass, slot_name)
        while (value := self.slots[i]) is None:
            signal(make_slot_unbound_error(self, slot_name))
        return value

    def slot_value_set(self, slot_name: "ZVal", value: "ZVal"):
        i = slot_location(self.klass, slot_name)
        self.slots[i] = value


@dataclass
class StructObj:
    klass: "ZVal"
    slots: tuple["ZVal", ...]


@dataclass
class Symbol:
    def __init__(self, name: str, mod: Module):
        assert name not in mod.symbols
        self.name = name
        self.mod = mod

    def call(self, *args: "ZVal") -> list["ZVal"]:
        data = self.data
        assert data.function is not None
        return funcall(data.function, *args)

    def call_setf(self, *args: "ZVal") -> list["ZVal"]:
        data = self.data
        assert data.setf is not None
        return funcall(data.setf, *args)

    @property
    def data(self) -> SymbolData:
        return self.mod.symbols[self.name]

    def __eq__(self, other):
        return self is other

    def __hash__(self) -> int:
        return hash((self.mod.name, self.name))

    def __repr__(self) -> str:
        if isinstance(self.mod, ModuleKeyword):
            return f":{self.name}"
        elif self.data.exported:
            return f"{self.mod.name}:{self.name}"
        else:
            return f"{self.mod.name}::{self.name}"


ZVal = (
    int
    | str
    | tuple["ZVal", "ZVal"]
    | BuiltInFunction
    | Module
    | SimpleVectorT
    | StandardObj
    | StructObj
    | Symbol
)
Nil = Z["NIL"]
T = Z["T"]
Nil.data.value = Nil
T.data.value = T

### Define a helper for interning a symbol.

### Define helpers for making and using Z lists and simple vectors.


def list_iter(lst: ZVal) -> Generator[ZVal, None, None]:
    while True:
        if lst is Nil:
            return
        elif isinstance(lst, tuple):
            yield lst[0]
            lst = lst[1]
        else:
            signal(make_not_a_list_error(lst))


def make_list(*elems: ZVal) -> Symbol | tuple[ZVal, ZVal]:
    out = Nil
    for elem in reversed(elems):
        out = (elem, out)
    return out


### Define some of the basics for defining built-in-functions. These get
### redefined once we've bootstrapped, to call the appropriate functions in the
### Z package.


def signal(*args: ZVal) -> list[ZVal]:
    raise Exception(f"TODO: signal {args!r}")


def error(*args: ZVal) -> NoReturn:
    raise Exception(f"TODO: error {args!r}")


def make_bad_arguments_for_lambda_list_error(
    func: str, lambda_list: ZVal, args: tuple[ZVal, ...]
) -> ZVal:
    raise Exception(
        f"TODO: make_bad_arguments_for_lambda_list_error {func} {lambda_list} {args}"
    )


def make_missing_slot_error(klass: ZVal, slot_name: ZVal) -> ZVal:
    raise Exception(f"TODO: make_missing_slot_error {klass} {slot_name}")


def make_not_instance_slot_error(klass: ZVal, slot: ZVal) -> ZVal:
    raise Exception(f"TODO: make_not_instance_slot_error {klass} {slot}")


def make_not_a_function_error(value: ZVal) -> ZVal:
    raise Exception(f"TODO: make_not_a_function_error {value}")


def make_not_a_list_error(value: ZVal) -> ZVal:
    raise Exception(f"TODO: make_not_a_list_error {value}")


def make_slot_unbound_error(obj: ZVal, slot: ZVal) -> ZVal:
    raise Exception(f"TODO: make_slot_unbound_error {obj} {slot}")


def def_builtin_function(
    name: str,
    *,
    setf: bool = False,
) -> Callable[[Callable[..., ZVal | list[ZVal]]], BuiltInFunction]:
    """
    A decorator for defining a built-in-function.
    """

    def wrapper(func) -> BuiltInFunction:
        assert func.__type_params__ == ()
        arg_spec = getfullargspec(func)
        assert arg_spec.defaults == None or all(
            value is None for value in arg_spec.defaults
        ), arg_spec
        assert arg_spec.kwonlydefaults == None or all(
            value is None for value in arg_spec.kwonlydefaults.values()
        ), arg_spec
        assert func.__code__.co_posonlyargcount == 0
        n_pos = (
            len(arg_spec.args)
            if arg_spec.defaults is None
            else (len(arg_spec.args) - len(arg_spec.defaults))
        )
        kwonlydefaults = (
            {} if arg_spec.kwonlydefaults is None else arg_spec.kwonlydefaults
        )

        for arg in arg_spec.args[:n_pos]:
            assert arg_spec.annotations[arg] == ZVal
        for arg in arg_spec.args[n_pos:]:
            assert arg_spec.annotations[arg] == ZVal | None
        if arg_spec.varargs is not None:
            assert arg_spec.annotations[arg_spec.varargs] == ZVal
        for arg in arg_spec.kwonlyargs:
            if arg in kwonlydefaults:
                assert arg_spec.annotations[arg] == ZVal | None
            else:
                assert arg_spec.annotations[arg] == ZVal
        if arg_spec.varkw is not None:
            assert arg_spec.varkw == "_"

        lambda_list = arg_spec.args[:n_pos]
        if n_pos != len(arg_spec.args):
            lambda_list += ["&OPTIONAL"]
            lambda_list += arg_spec.args[n_pos:]
        if arg_spec.kwonlyargs != []:
            lambda_list += ["&KEY"]
            lambda_list += arg_spec.kwonlyargs
        if arg_spec.varkw is not None:
            lambda_list += ["&ALLOW-OTHER-KEYS"]
        if arg_spec.varargs is not None:
            lambda_list += ["&REST", arg_spec.varargs]
        lambda_list = make_list(*lambda_list)

        def wrapper(*z_args: ZVal) -> list[ZVal]:
            kwargs = {}

            ## If there aren't enough arguments for the required arguments,
            ## fail.
            while len(z_args) < n_pos:
                signal(
                    make_bad_arguments_for_lambda_list_error(name, lambda_list, z_args)
                )
            ## If there are more arguments than can be consumed by the required
            ## and optional arguments, and there are not &REST or &KEY
            ## arguments, fail.
            if (
                len(z_args) > len(arg_spec.args)
                and arg_spec.varargs is None
                and arg_spec.kwonlyargs != []
            ):
                signal(
                    make_bad_arguments_for_lambda_list_error(name, lambda_list, z_args)
                )
            ## If there are rest arguments, we can pass through the entire
            ## argument list. If not (i.e., if there's &KEY arguments), we want
            ## to chop it off if it goes over.
            if arg_spec.varargs is None:
                args = z_args[: len(arg_spec.args)]
            else:
                args = z_args

            ## If there are keyword arguments, we have to process them.
            if arg_spec.kwonlyargs:
                kwable_args = z_args[len(arg_spec.args) :]
                while len(kwable_args):
                    if len(kwable_args) < 2:
                        raise Exception(
                            f"TODO: malformed kwarg list {kwable_args} for {name}"
                        )
                    kwd = kwable_args[0]
                    arg = kwable_args[1]
                    kwable_args = kwable_args[2:]
                    assert isinstance(kwd, Symbol) and kwd.mod is KEYWORD
                    kwd_name = kwd.name.lower().replace("-", "_")
                    if kwd_name in arg_spec.kwonlyargs:
                        ## The keyword was known.
                        if kwd_name in kwargs:
                            raise Exception(f"TODO: duplicate keyword {kwd} for {name}")
                        kwargs[kwd_name] = arg
                    elif arg_spec.varkw is not None:
                        ## The keyword was unknown, but we have
                        ## &ALLOW-OTHER-KEYS, so don't emit an error.
                        pass
                    else:
                        ## The keyword was unknown.
                        raise Exception(f"TODO: unknown keyword {kwd} for {name}")

            out = func(*args, **kwargs)
            if not isinstance(out, list):
                out = [out]
            return out

        data = Z[name].data
        data.exported = True
        if setf:
            assert data.setf is None
            data.setf = wrapper
        else:
            assert data.function is None
            data.function = wrapper
        return wrapper

    return wrapper


@def_builtin_function("FUNCALL")
def funcall(func: ZVal, *args: ZVal) -> list[ZVal]:
    while True:
        if isinstance(func, Callable):
            return func(*args)
        signal(make_not_a_function_error(func))


### Define slot access.


def slot_location(klass: ZVal, slot_name: ZVal) -> int:
    assert isinstance(slot_name, Symbol)

    if klass == ClassStandardClass and slot_name is Z["EFFECTIVE-SLOTS"]:
        for i, slot in enumerate(SlotsStandardClass):
            (name,) = Z["SLOT-DEFINITION/NAME"].call(slot)
            if name is Z["EFFECTIVE-SLOTS"]:
                return i
        raise Exception("slot_location: StandardClass does not have EFFECTIVE-SLOTS?")
    else:
        (slots,) = Z["CLASS/EFFECTIVE-SLOTS"].call(klass)
        assert isinstance(slots, SimpleVectorT)
        for i, slot in enumerate(slots.elems):
            (name,) = Z["SLOT-DEFINITION/NAME"].call(slot)
            if name is slot_name:
                (allocation,) = Z["SLOT-DEFINITION/ALLOCATION"].call(slot)
                if allocation is not KEYWORD["INSTANCE"]:
                    error(make_not_instance_slot_error(klass, slot_name))
                return i
        error(make_missing_slot_error(klass, slot_name))


@def_builtin_function("SLOT-BOUND?")
def slot_boundp(obj: ZVal, slot_name: ZVal) -> list[ZVal]:
    if (
        isinstance(obj, StandardObj)
        and Z["CLASS-OF"].call(obj.klass)[0] is ClassStandardClass
    ):
        return [obj.slot_boundp(slot_name)]
    else:
        return Z["SLOT-BOUND?-USING-CLASS"].call(
            Z["CLASS-OF"].call(obj)[0], obj, slot_name
        )


@def_builtin_function("SLOT-UNBIND")
def slot_unbind(obj: ZVal, slot_name: ZVal) -> list[ZVal]:
    if (
        isinstance(obj, StandardObj)
        and Z["CLASS-OF"].call(obj.klass)[0] is ClassStandardClass
    ):
        obj.slot_unbind(slot_name)
        return [Nil]
    else:
        return Z["SLOT-UNBIND-USING-CLASS"].call(
            Z["CLASS-OF"].call(obj)[0], obj, slot_name
        )


@def_builtin_function("SLOT-VALUE")
def slot_value(obj: ZVal, slot_name: ZVal) -> list[ZVal]:
    if (
        isinstance(obj, StandardObj)
        and Z["CLASS-OF"].call(obj.klass)[0] is ClassStandardClass
    ):
        return [obj.slot_value(slot_name)]
    else:
        return Z["SLOT-VALUE-USING-CLASS"].call(
            Z["CLASS-OF"].call(obj)[0], obj, slot_name
        )


@def_builtin_function("SLOT-VALUE", setf=True)
def slot_value_setf(obj: ZVal, slot_name: ZVal, value: ZVal) -> list[ZVal]:
    if (
        isinstance(obj, StandardObj)
        and Z["CLASS-OF"].call(obj.klass)[0] is ClassStandardClass
    ):
        obj.slot_value_set(slot_name, value)
        return [value]
    else:
        return Z["SLOT-VALUE-USING-CLASS"].call_setf(
            Z["CLASS-OF"].call(obj)[0], obj, slot_name, value
        )


@def_builtin_function("SLOT-EXISTS?")
def slot_exists(obj: ZVal, slot_name: ZVal) -> list[ZVal]:
    if (
        isinstance(obj, StandardObj)
        and Z["CLASS-OF"].call(obj.klass)[0] is ClassStandardClass
    ):
        return [obj.slot_exists(slot_name)]
    else:
        return Z["SLOT-EXISTS?-USING-CLASS"].call(
            Z["CLASS-OF"].call(obj)[0], obj, slot_name
        )


### The class-of function.


@def_builtin_function("CLASS-OF")
def class_of(obj: ZVal) -> list[ZVal]:
    if isinstance(obj, int):
        return Z["FIND-CLASS"].call(Z["INT"])
    elif isinstance(obj, str):
        return Z["FIND-CLASS"].call(Z["STR"])
    elif isinstance(obj, tuple):
        return Z["FIND-CLASS"].call(Z["CONS"])
    elif isinstance(obj, Callable):
        return Z["FIND-CLASS"].call(Z["BUILT-IN-FUNCTION"])
    elif isinstance(obj, Module):
        return Z["FIND-CLASS"].call(Z["MODULE"])
    elif isinstance(obj, SimpleVectorT):
        return Z["FIND-CLASS"].call(Z["SIMPLE-VECTOR-T"])
    elif isinstance(obj, StandardObj):
        return [obj.klass]
    elif isinstance(obj, StructObj):
        return [obj.klass]
    elif isinstance(obj, Symbol):
        if obj == Nil:
            return Z["FIND-CLASS"].call(Z["NULL"])
        else:
            return Z["FIND-CLASS"].call(Z["SYMBOL"])


### Checking for subclassiness and subspecializeriness.


def subclassp(child: ZVal, parent: ZVal) -> bool:
    """
    Returns whether CHILD is a subclass of PARENT.
    """

    return any(
        klass is parent
        for klass in list_iter(Z["CLASS-PRECEDENCE-LIST"].call(child)[0])
    )


# TODO: sub-specializer-p

### Define standard-class's accessors.


## TODO: The class accessor functions are _not_ generic-functions, but should be.
def define_class_accessors(slot):
    @def_builtin_function(f"CLASS/{slot}")
    def class_reader(klass: ZVal) -> list[ZVal]:
        return Z["SLOT-VALUE"].call(klass, Z[slot])

    @def_builtin_function(f"CLASS/{slot}", setf=True)
    def class_writer(klass: ZVal, value: ZVal) -> list[ZVal]:
        return Z["SLOT-VALUE"].call_setf(klass, Z[slot], value)

    _ = class_reader, class_writer


for slot in [
    "NAME",
    "DIRECT-SUPERCLASSES",
    "DIRECT-SLOTS",
    "PRECEDENCE-LIST",
    "EFFECTIVE-SLOTS",
    "DIRECT-SUBCLASSES",
    "DIRECT-METHODS",
]:
    define_class_accessors(slot)

del define_class_accessors


### Define a helper function for defclass.


def defclass(
    name: Symbol,
    direct_superclasses: list[Symbol],
    *direct_slots: ZVal,
    **options: ZVal,
) -> StandardObj:
    def canonicalize_direct_slot(direct_slot: ZVal) -> ZVal:
        raise Exception("TODO: canonicalize_direct_slot")

    def canonicalize_defclass_option(name: str, value: ZVal) -> list[ZVal]:
        raise Exception("TODO: canonicalize_defclass_option")

    assert len(options) == 0  # TODO
    out = Z["ENSURE-CLASS"].call(
        name,
        KEYWORD["DIRECT-SUPERCLASSES"],
        SimpleVectorT(
            *(Z["FIND-CLASS"].call(class_name)[0] for class_name in direct_superclasses)
        ),
        KEYWORD["DIRECT-SLOTS"],
        SimpleVectorT(
            *(canonicalize_direct_slot(direct_slot) for direct_slot in direct_slots)
        ),
        *(arg for k, v in options for arg in canonicalize_defclass_option(k, v)),
    )
    assert len(out) == 1
    assert isinstance(out[0], StandardObj)
    return out[0]


### Define find-class.


@def_builtin_function("FIND-CLASS")
def find_class(class_name: ZVal, error: ZVal | None = None) -> ZVal:
    if error is None:
        error = T
    assert isinstance(class_name, Symbol), class_name

    if class_name.data.klass is None:
        if error is Nil:
            return Nil
        else:
            raise Exception(f"TODO: find_class {class_name}")
    else:
        return class_name.data.klass


### Define ensure-class.


@def_builtin_function("ENSURE-CLASS")
def ensure_class(
    class_name: ZVal,
    *all_keys: ZVal,
    metaclass: ZVal | None = None,
    direct_superclasses: ZVal | None = None,
    direct_slots: ZVal | None = None,
    **_: None,
) -> ZVal:
    if metaclass is None:
        metaclass = ClassStandardClass
    if direct_superclasses is None:
        direct_superclasses = SimpleVectorT()
    if direct_slots is None:
        direct_slots = SimpleVectorT()
    assert isinstance(class_name, Symbol)
    assert isinstance(direct_superclasses, SimpleVectorT)
    assert isinstance(direct_slots, SimpleVectorT)

    if Z["FIND-CLASS"].call(class_name, Nil)[0] is not Nil:
        raise Exception(f"TODO: redefine the class {class_name}")

    if metaclass is ClassStandardClass:
        if len(direct_superclasses.elems) == 0:
            direct_superclasses = SimpleVectorT(
                Z["FIND-CLASS"].call(Z["STANDARD-OBJECT"])[0]
            )
        slots = []
        for args in direct_slots.elems:
            assert isinstance(args, SimpleVectorT)
            (slot,) = Z["MAKE-DIRECT-SLOT-DEFINITION"].call(*args.elems)
            slots.append(slot)

        klass = StandardObj(ClassStandardClass)
        Z["CLASS/NAME"].call_setf(klass, class_name)
        Z["CLASS/DIRECT-SUBCLASSES"].call_setf(klass, SimpleVectorT())
        Z["CLASS/DIRECT-SUPERCLASSES"].call_setf(klass, direct_superclasses)
        Z["CLASS/DIRECT-METHODS"].call_setf(klass, SimpleVectorT())
        Z["CLASS/DIRECT-SLOTS"].call_setf(klass, SimpleVectorT(*slots))

        for sup in direct_superclasses.elems:
            (sup_subs,) = Z["CLASS/DIRECT-SUBCLASSES"].call(sup)
            assert isinstance(sup_subs, SimpleVectorT)
            Z["CLASS/DIRECT-SUBCLASSES"].call_setf(
                sup, SimpleVectorT(*sup_subs.elems, klass)
            )

        for slot in slots:
            (name,) = Z["SLOT-DEFINITION/NAME"].call(slot)
            (readers,) = Z["DIRECT-SLOT-DEFINITION/READERS"].call(slot)
            (writers,) = Z["DIRECT-SLOT-DEFINITION/WRITERS"].call(slot)
            assert isinstance(readers, SimpleVectorT)
            assert isinstance(writers, SimpleVectorT)
            for reader in readers.elems:
                Z["ADD-READER-METHOD"].call(klass, reader, name)
            for writer in writers.elems:
                Z["ADD-WRITER-METHOD"].call(klass, writer, name)

        standard_class_finalize_inheritance(klass)
        # Z["CLASS/PRECEDENCE-LIST"].call_setf(klass, Nil)
        # Z["CLASS/EFFECTIVE-SLOTS"].call_setf(klass, Nil)
        raise Exception("TODO: ensure_class")
    else:
        klass = Z["MAKE-INSTANCE"].call(
            metaclass, KEYWORD["NAME"], class_name, *all_keys
        )[0]

    class_name.data.klass = klass
    return klass


### Define the slot-definition metaobjects.
###
### TODO: I'm really not a fan of these being vectors instead some kind of actual object.


def make_direct_slot_definition(
    name: ZVal,
    *,
    initargs: ZVal | None = None,
    initform: ZVal | None = None,
    initfunction: ZVal | None = None,
    allocation: ZVal | None = None,
    readers: ZVal | None = None,
    writers: ZVal | None = None,
):
    return SimpleVectorT(
        name,
        initargs if initargs is not None else Nil,
        initform if initform is not None else Nil,
        initfunction if initfunction is not None else Nil,
        allocation if allocation is not None else KEYWORD["INSTANCE"],
        readers if readers is not None else Nil,
        writers if writers is not None else Nil,
    )


def make_effective_slot_definition(
    name: ZVal,
    *,
    initargs: ZVal | None = None,
    initform: ZVal | None = None,
    initfunction: ZVal | None = None,
    allocation: ZVal | None = None,
):
    return SimpleVectorT(
        name,
        initargs if initargs is not None else Nil,
        initform if initform is not None else Nil,
        initfunction if initfunction is not None else Nil,
        allocation if allocation is not None else KEYWORD["INSTANCE"],
    )


@def_builtin_function("SLOT-DEFINITION/NAME")
def slot_definition_name(slot_definition: ZVal) -> list[ZVal]:
    assert isinstance(slot_definition, SimpleVectorT)
    return [slot_definition.elems[0]]


@def_builtin_function("SLOT-DEFINITION/INITARGS")
def slot_definition_initargs(slot_definition: ZVal) -> list[ZVal]:
    assert isinstance(slot_definition, SimpleVectorT)
    return [slot_definition.elems[1]]


@def_builtin_function("SLOT-DEFINITION/INITFORM")
def slot_definition_initform(slot_definition: ZVal) -> list[ZVal]:
    assert isinstance(slot_definition, SimpleVectorT)
    return [slot_definition.elems[2]]


@def_builtin_function("SLOT-DEFINITION/INITFUNCTION")
def slot_definition_initfunction(slot_definition: ZVal) -> list[ZVal]:
    assert isinstance(slot_definition, SimpleVectorT)
    return [slot_definition.elems[3]]


@def_builtin_function("SLOT-DEFINITION/ALLOCATION")
def slot_definition_allocation(slot_definition: ZVal) -> list[ZVal]:
    assert isinstance(slot_definition, SimpleVectorT)
    return [slot_definition.elems[4]]


@def_builtin_function("DIRECT-SLOT-DEFINITION/READERS")
def direct_slot_definition_readers(slot_definition: ZVal) -> list[ZVal]:
    assert isinstance(slot_definition, SimpleVectorT)
    return [slot_definition.elems[5]]


@def_builtin_function("DIRECT-SLOT-DEFINITION/WRITERS")
def direct_slot_definition_writers(slot_definition: ZVal) -> list[ZVal]:
    assert isinstance(slot_definition, SimpleVectorT)
    return [slot_definition.elems[6]]


## TODO: Accessors.

### Define finalize-inheritance.


### Bootstrap the object system.

## Define the STANDARD-CLASS slots.
SlotsStandardClass = [
    make_effective_slot_definition(
        Z["NAME"],
    ),
    make_effective_slot_definition(
        Z["DIRECT-SUPERCLASSES"],
    ),
    make_effective_slot_definition(
        Z["DIRECT-SLOTS"],
    ),
    make_effective_slot_definition(
        Z["PRECEDENCE-LIST"],
    ),
    make_effective_slot_definition(
        Z["EFFECTIVE-SLOTS"],
    ),
    make_effective_slot_definition(
        Z["DIRECT-SUBCLASSES"],
    ),
    make_effective_slot_definition(
        Z["DIRECT-METHODS"],
    ),
]

## Define the STANDARD-CLASS object.
ClassStandardClass = StandardObj.__new__(StandardObj)
ClassStandardClass.klass = ClassStandardClass
ClassStandardClass.slots = [None] * len(SlotsStandardClass)
Z["CLASS/EFFECTIVE-SLOTS"].call_setf(
    ClassStandardClass, SimpleVectorT(*SlotsStandardClass)
)

## Define the class T.
ClassT = StandardObj(ClassStandardClass)
Z["CLASS/NAME"].call_setf(ClassT, Z["T"])
Z["CLASS/DIRECT-SUBCLASSES"].call_setf(ClassT, SimpleVectorT())
Z["CLASS/DIRECT-SUPERCLASSES"].call_setf(ClassT, SimpleVectorT())
Z["CLASS/DIRECT-METHODS"].call_setf(ClassT, SimpleVectorT())
Z["CLASS/DIRECT-SLOTS"].call_setf(ClassT, SimpleVectorT())
Z["CLASS/PRECEDENCE-LIST"].call_setf(ClassT, SimpleVectorT())
Z["CLASS/EFFECTIVE-SLOTS"].call_setf(ClassT, SimpleVectorT())
Z["T"].data.klass = ClassT

## Define the STANDARD-OBJECT class.
defclass(name=Z["STANDARD-OBJECT"], direct_superclasses=[Z["T"]])

print(ClassT)
print(ClassStandardClass)
exit()

## Re-define the STANDARD-CLASS object.
# print(Z["CLASS/NAME"].call(ClassStandardClass))
