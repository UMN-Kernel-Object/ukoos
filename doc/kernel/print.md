# The `print()` and `format()` functions

The ukoOS kernel provides different printing functions than one may be used to from userspace C.
They more closely resemble what's provided by Python or Rust, although they have some differences because they can't rely on runtime type information.
(If we *can* get runtime type information in the future, it would absolutely make sense to use it here.)

These functions are:

```c
char *format(const char *fmt, ...);
void print(const char *fmt, ...);
```

Both functions take the same arguments, the only difference is whether they print the string immediately or return it in a heap-allocated buffer.

> **TODO**: `print` shouldn't print "immediately" either; it should print to a ringbuffer, so that it can be used in e.g. an interrupt handler.

The first argument is a format string.
This string is composed of chunks of literal characters, format directives, and curly brace escapes.
We can see all three in a print call like:

```c
void foo(bool b, u32 n) {
    print("f({bool}) = {{ 1, 2, {u32:#010x} }}", b, n);
}
```

In this call, the string breaks down as follows:

```
                            f({bool}) = {{1, 2, {u32:#010x}}}
               literal "f(" ┴┘└────┤└──┤└┤└────┤└─────────┤└┤
format directive without arguments ┘   │ │     │          │ │
                        literal ") = " ┘ │     │          │ │
                             escaped '{' ┘     │          │ │
                             literal "1, 2, "  ┘          │ │
                          format directive with arguments ┘ │
                                                escaped '}' ┘
```

Literal chunks are printed as-is, and curly brace escapes print as the characters they're escaping.
Format directives print content computed at print-time, typically content computed from the arguments to `print` or `format`.

Format directives are split into two parts; inside the curly braces, there's a type name and optional arguments, separated by a colon (`:`).

## List of format directives

### `i8`, `i16`, `i32`, `i64`, `isize`, `u8`, `u16`, `u32`, `u64`, `usize`

These directives expect the appropriate C type in the arguments to `print` or `format`.

These directives print numbers.
They take a variety of arguments:

- `#`: Prints the a marker for the base before the sign (`0b`, `0o`, or `0x`).
- `0`: Left-pads the number with `0` (after the sign) instead of with ` ` (before the sign).
- a number: Left-pads the number until it is _at least_ this length.
- `b`, `o`, `x`: Prints the number in binary, octal, or hex.

### `paddr`, `uaddr`, `uptr`

These directives expect the appropriate C type in the arguments to `print` or `format`.

These directives print the address or the address part of the pointer.
They act like `usize`, except they default to having arguments of `#018x`.

### `bool`

This directive expects a C `bool` in the arguments to `print` or `format`.

It prints either `true` or `false`, corresponding to the value.
This directive does not take any arguments.

### `cstr`

This directive expects a C `const char *` in the arguments to `print` or `format`.

It prints it as a null-terminated string.
This directive does not take any arguments.

### `indent`

This directive expects a C `usize` in the arguments to `print` or `format`.

It prints that number of space characters (` `).
This directive does not take any arguments.

### `va`

This directive expects a C `const char*` and a C `va_list` in the arguments to `print` or `format`.

It prints the content that `format` would print if that format string and those arguments were passed to it.
This directive does not take any arguments.
