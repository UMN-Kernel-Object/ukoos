# zpy

A bootstrap interpreter for the Z language, written in Python.
See the documentation for more about the language.

This interpreter runs on the host at build-time, and is used to interpret the `zz` compiler and run it on itself, producing the binary we actually ship.

The Python parts actually just implement a Common Lisp-like syntax, without proper support for the object system.
This is used to interpret code written in that syntax and subset of the language that bootstraps the object system and implements a proper reader.
