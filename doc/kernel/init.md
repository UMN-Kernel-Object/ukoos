# Init Functions

The `init.h` header defines a mechanism to register code to run during boot.
Blocks of code (initializers) can be registered to run during boot with the `DEFINE_INIT` macro.

This macro takes a priority as well.
Initializers are run from the lowest priority to highest.
If two initializers have the same priority, they may be run in any order.

## Priorities

- `INIT_REGISTER_DRIVERS`: The priority level at which drivers should register themselves.
