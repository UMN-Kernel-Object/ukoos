# Z Concepts

This page describes important concepts in Z and its standard library that do not make sense to attach to a docstring.
It's not necessary for a Z learner to read and understand everything here before writing their first program, but understanding these concepts is important to achieve mastery.

## Syntax

### Reading and parsing

## Compilation

Z is a compiled language, but it has `eval`, a REPL, and supports interactive development.
This might seem paradoxical, but the implementation is actually relatively straightforward.

Most Z code is compiled a function at a time.

## Packages and symbols

### Exported symbols

### Keywords

## Structs and classes

## Functions and methods

## Protocols

### Streams

A stream is an object whose class inherits from `Z:STREAM`.
Streams should have methods for:

- `Z:STREAM/READ`
- `Z:STREAM/TRY-READ`
- `Z:STREAM/EMPTY?`
- `Z:STREAM/TYPE`

## The metaobject protocol

Like some other object-oriented languages, Z has a metaobject protocol.
In simple terms, this means:

- In Z, classes are ordinary (first-class) values; they can be passed as arguments to functions, stored in data structures, etc.
- The same is true of various other parts of the object system (metaobjects): methods, slot definitions, etc.
- The classes of metaobjects are not special in any way -- they're just ordinary classes too.
- Programmers can define their own classes to make their own metaobjects.
- A standard protocol exists to allow programmers to specify the behavior of these metaobjects.
