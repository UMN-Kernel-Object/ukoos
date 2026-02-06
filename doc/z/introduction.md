# Z Introduction

The Z language is a compiled, garbage-collected, object-oriented language, used for ukoOS's userspace.

Brief comparisons to some popular languages:

- **Python** (in particular, as implemented by CPython):
  - This is the language that's most similar to Z if you consider both syntax and semantics.
    - Both are gradually-typed, though Z also checks type annotations at runtime.
    - Interactive development is supported in Z, kinda like Jupyter.
  - Unlike Python, Z is ahead-of-time compiled: currently, there is no interpreter, even for the REPL.
  - Unlike Python, Z has macros.
- **C**, **C++**, and **Rust** (in particular, as implemented by GCC, LLVM, and rustc):
  - These would be the normal choices for the "default language" in userspace.
  - These languages avoid garbage collection for performance reasons.
    Z allows writing low-level code that manually manages memory, but it's not the default.
  - Unlike these languages, Z supports interactive development.
    Testing changes to a program can be done without stopping running instances of it.
  - Unlike C and C++, Z's macro system allows writing macros in Z (like proc macros in Rust).
- **Go:**
  - Like Go, Z is both compiled and garbage-collected.
  - Like Go, Z's method definitions are separate from its type definitions.
  - Like Go, Z has async without "function coloring."
  - Unlike Go, Z has a REPL and supports interactive development.
  - Unlike Go, Z has operator overloading and macros.
  - Unlike Go, Z has inheritance and open recursion.
- **Java** (in particular, as implemented by OpenJDK):
  - This is another garbage-collected object-oriented language sometimes used for low-level programming (e.g. in high-performance databases).
  - Z's support for low-level programming is broadly similar to Java's (e.g. `sun.misc.Unsafe`).
  - Like Java, Z allows loading code at runtime and safely modifying running code.
  - Unlike Java, Z does not use a JIT compiler.
    To be precise, Z does dynamic compilation but not dynamic recompilation -- once a function's machine code is generated, it is not recompiled at runtime without the programmer writing code to do this.)
    - This leads to more predictable performance and faster short-running tasks.
  - Unlike Java, Z supports multiple dispatch and multiple inheritance.
- **JavaScript** (in particular, as implemented by QuickJS and V8):
  - Like JavaScript's HMR, Z supports interactive development.
  - Like TypeScript, Z supports gradual typing.
    - Unlike TypeScript, Z checks the type annotations at runtime.
  - Unlike JavaScript, Z is strongly typed (it does not perform implicit coercions).
  - Unlike JavaScript, Z does not have a "baseline" interpreter.
  - Unlike the V8 implementation of JavaScript, Z does not have a JIT compiler.
  - Unlike the QuickJS implementation of JavaScript, Z compiles to machine code.
  - Unlike JavaScript, Z doesn't have "function coloring."

Brief comparisons to some other languages that are similar to Z:

- **Common Lisp:**
  - Z directly takes most of its semantics from Common Lisp.
  - The biggest departure from Common Lisp is the use of shrubbery syntax.
  - Unlike Common Lisp, Z supports delimited continuations as a control-flow primitives.
    - Like `CL:THROW`, these are a low-level primitive; most code just does IO and doesn't worry about control effects.
    - This *does* spill into `CL:UNWIND-PROTECT` -- when we capture a delimited continuation, we've exited its dynamic scope, but calling the continuation re-enters it.
- **Racket**, **Rhombus**, and **Scheme:**
  - Z shares the shrubbery notation syntax with Rhombus, and uses syntax objects rather than simple data structures.
  - Unlike Scheme-family languages, Z has mutable data structures, and they're usually the "default" ones to use.
