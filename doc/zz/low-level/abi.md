# ABI

Choosing a good ABI is critical for the performance of any language.
However, Z is an expressive and high-level language, which constrains the ABI to one that can support its features.
Some of the more complicated features of Z practically necessitate an ABI that puts a strict cap on performance.

However, ZZ is designed to allow for aggressive interprocedural optimization, which allows the most general (and therefore slowest) mechanisms to be removed when the callee of a function is known, even when that function isn't completely inlined.

This is done via the worker/wrapper transform: every source-level function generates two optimizer-level functions, a "worker" and a "wrapper."

When other functions try to call the function, their calls are made to the wrapper, which uses the standard ABI, since the caller might not know which function it is.
However, the wrapper is marked to always be inlined into its caller.
When it is inlined, the call to the worker can use a non-standard ABI.
The wrapper can also perform type-checks on the arguments and other precondition checks, allowing these to be eliminated when they're statically known from the context of the call.

The worker contains the actual code generated from body of the source-level function.
Becaues every call to the worker goes through the wrapper, the worker can be compiled assuming that the preconditions from the worker are true, without re-checking them.
Any type-checks on the return values and other post-conditions can similarly be checked in the worker, and assumed in the wrapper and thus the caller.

## Standard ABI

The standard ABI is the fully general ABI, which can be used for any call between source-level functions.

### Register usage

We follow the RISC-V LP64D ABI.

- The scalar registers `a0`-`a7`, and `t0`-`t6` are caller-save.
- The scalar registers `ra`, `sp` and `s0`-`s11` are callee-save.
- The scalar registers `zero`, `gp`, and `tp` are assumed to be immutable.
- The floating-point registers `fa0`-`fa7` and `ft0-ft11` are caller-save.
- The floating-point registers `fs0`-`fs11` are callee-save.
- All vector registers are caller-save.

Some of these registers get particular uses.

- `ra`: The return address.
- `sp`: A pointer to the current stack frame.
- `s0`: The GC's nursery heap bump pointer.
- `s1`: The GC's nursery remembered set bump pointer.
- `gp`: The global state, described below.
- `tp`: The thread state, described below.

### Arguments

- All arguments are passed in the `T` representation.
- `a0` contains the number of arguments as an unsigned 64-bit integer.
- `a1`-`a6` contain the first six arguments, if present.
  If not, their value will be ignored.
- If there are more than six arguments, `a7` contains a cons-list of the remaining arguments.
  If not, its value will be ignored.

### Returns

- All return values are returned in the `T` representation.
- `a0` contains the number of return values as an unsigned 64-bit integer.
- `a1` contains the first return value if it exists, or nil if not.
- `a2`-`a6` contain the next five return values, if present.
  If not, their value will be ignored.
- If there are more than six return values, `a7` contains a cons-list of the remaining return values.
  If not, its value will be ignored.

## `gp` and `tp`

The `gp` and `tp` registers point to structures that contain all the global and thread-local state, making it possible to relocate them.
These each use a layout like an ordinary `structure-object`, simplifying the garbage collector.
They are not full `structure-object`s, however, and their "class" cannot be redefined.

The global state has the following fields:

- `alloc-young`: A pointer to the function that allocates into the GC's young heap.
- `alloc-old`: A pointer to the function that allocates into the GC's old heap.
- `collect`: A pointer to the function that performs a garbage collection.
- `modules`: A hashtable from module names to modules.
- TODO

The thread state has the following fields:

- `unwind-stack`: An `unwind-frame` object, which forms the stack of unwind frames.
- TODO
