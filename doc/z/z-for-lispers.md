# Z for Lispers

Z started life as a "modernized" Common Lisp variant, so lots of parts of it will be familiar to Common Lisp programmers.
Some notable Common Lisp-specific things are mentioned here.

- An [iterate](https://iterate.common-lisp.dev/)-like loop syntax is present instead of a `CL:LOOP`-like one.
- Many names are "modernized" to be more familiar to non-CL programmers.
  A few instances of this that might otherwise be confusing:
  - The equivalent of a CL package is a Z module.
  - The equivalent of a CL system is a Z package.
- Most modules are loaded from files with a scheme similar to `package-inferred-system`.
- [Package-local nicknames](https://gist.github.com/phoe/2b63f33a2a4727a437403eceb7a6b4a3) are supported.
- Most polymorphic functions are generic functions, similar to what [GENERIC-CL](https://gutev.dev/generic-cl/) does.
- Z tracks dependencies between functions, so callers automatically get recompiled when an `INLINE` function is redefined.
- Z's compiler has a more sophisticated inliner than most CL implementations, so `INLINE` is applied to function definitions by default.
  - There is a `MUST-INLINE` declaration to override the inliner's decisions.
- The type system has been overhauled; many types are spelled differently.
  - The more sophisticated inliner allows other optimizations around type-checking to make `SATISFIES` types more efficient.
- There are predefined types to correspond to common machine quantities:
  - `{i,u}{8,16,32,64}`: integers, signed and unsigned
  - `f{16,32,64}`: IEEE754 floating-point types
  - `bf16`: "Brain floats"
- The array type hierarchy has been overhauled.
  - All array types are parameterized by an optional element type.
  - `array`'s Common Lisp equivalent is a rank-1 array which is not adjustable, has no fill pointer, and is not displaced.
    - `array` can also accept a length, e.g. `(array f32 4)`.
  - `vector`'s Common Lisp equivalent is a rank-1 array which is adjustable, has a fill pointer, and is not displaced.
  - `slice`'s Common Lisp equivalent is a rank-1 array which is not adjustable, has no fill pointer, and is displaced.
    - `slice` can also accept a length, e.g. `(slice f32 4)`.
  - `ndarray`'s Common Lisp equivalent is an arbitrary-rank array which is adjustable, has a fill pointer, and is not displaced.
    - `slice` can also accept a rank or a list of dimensions, any of which can be wildcarded with `*`, e.g. `(ndarray f32 2)` or `(ndarray f32 (* * 3))`.
- Strings are not a subtype of `(array character)`.
  Strings are always UTF-8, and get a dedicated type.
