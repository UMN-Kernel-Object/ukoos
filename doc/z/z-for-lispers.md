# Z for Lispers

Z started life as a "modernized" Common Lisp variant, so lots of parts of it will be familiar to Common Lisp programmers.
Some notable Common Lisp-specific things are mentioned here.

- An [iterate](https://iterate.common-lisp.dev/)-like loop syntax is present instead of a `CL:LOOP`-like one.
- The equivalent of a CL package is a Z module.
  The equivalent of a CL system is a Z package.
- Most modules are loaded from files with a scheme similar to `package-inferred-system`.
- [Package-local nicknames](https://gist.github.com/phoe/2b63f33a2a4727a437403eceb7a6b4a3) are supported.
- Most polymorphic functions are generic functions, similar to what [GENERIC-CL](https://gutev.dev/generic-cl/) does.
