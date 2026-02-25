# Value representation

Currently, all values use the same representation (called "the `T` representation," because it can store any subtype of `T`, i.e., any value).

## The `T` representation

This representation is based on the *sofa* representation from "[What About the *Integer* Numbers?]," which allows storing either a small (30-bit) signed integer or a 4-byte-aligned pointer, while getting very cheap overflow checks and tag checks for common operations.

[What About the *Integer* Numbers?]: https://www.microsoft.com/en-us/research/wp-content/uploads/2022/07/int.pdf

The `T` representation also standardizes the layout of the memory object pointed to when a value is a pointer.
These pointers are always 8-byte-aligned (the extra potential tag bit is currently unused).
The memory object always starts with a pointer to the class of the value, followed by the actual data of the object.

It's worth calling out the particular layouts of the memory objects for a few classes.

### `array-{bf16,f{16,32,64}}`

TODO

### `array-{i,u}{8,16,32,64}`

TODO

### `array-t`

TODO

### `bf16`, `f{16,32,64}`

TODO

### `bigint`

TODO

### `cons`

TODO

### `fixint`

TODO

### `function`

TODO

### `null`

The `nil` constant is represented as a null pointer, making it cheap to branch on whether a value is `nil` or not.

### `standard-object`

TODO

### `str`

TODO

### `structure-object`

TODO

## Other representations

In the future, there are a few representations that might make sense to add:

- Representations for word-sized or smaller unboxed integers.
- A representation for untagged pointers.
- A representation for untagged integers (that might still be bigints).
- A representation for unboxed floating-point scalars.
- A representation for stack-allocated arrays.
