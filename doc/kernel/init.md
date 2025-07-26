# Init Functions

The `init.h` header defines a mechanism to register code to run during boot.
Blocks of code (initializers) can be registered to run during boot with the `DEFINE_INIT` macro.

This macro takes a priority as well.
Initializers are run from the lowest priority to highest.
If two initializers have the same priority, they may be run in any order.

### Example

```c
DEFINE_INIT(0) {
    print("A: This gets printed during boot!");
}

DEFINE_INIT(10) {
    print("B: This always gets printed after A");
}

DEFINE_INIT(10) {
    print("C: This might gets printed before or after B");
}
```

### Priorities with given semantics

- -20: Device classes should register themselves.
- -10: Devices should register themselves.
- 0: Drivers should initialize themselves.
