# Guards

RAII, or "resource acquisition is initialization," is a common pattern in C++, Rust, and similar languages.
With RAII, some object or resource being acquired (i.e., having a value of that type in a valid state) is tied to that object being initialized and deinitialized.

In C++, this is accomplished with constructors and destructors; in Rust, with privacy and the Drop trait.
In C, we can implement this with [the `cleanup` attribute](https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-cleanup-variable-attribute).
This attribute applies to a variable, and registers a function to run when the variable goes out of scope.

This is roughly equivalent to the Drop trait, with one limitation -- we can't easily move the variable to extend its lifetime past the scope it's declared in.
However, this is still useful enough for us to have RAII objects to manage access to resources.

The objects that manage access to the resource are called "guards."
These are usually used to handle locks and refcounts.

## Using a guard

Most types of guards will have a macro for acquiring the guard, typically named something like `FOO_GUARD`.
This macro takes a variable name (for the guarded resource) as the first argument, and some number of arguments afterwards.

Typically, there's another macro for declaring guarded variables, to make it harder to accidentally use them without acquiring the guard.
For example:

```c
#include <mutex.h>
#include <print.h>

struct foo {
    // Declare a mutex named bar and some variables protected by it.
    MUTEX_GUARDED(bar, {
        int baz;
        char *asdf;
    });

    // This variable is not protected by the mutex.
    int qwerty;
};

int foo_func(struct foo* foo) {
    int x = 3 * foo->qwerty;

    // Because of the MUTEX_GUARDED macro, we can't just write e.g.
    //     x += foo->baz;

    {
        MUTEX_GUARD(g, foo, bar);

        // We can access the variables through g instead, though!
        x += g->baz;

        print("{cstr}", g->asdf);
    }

    // Mutex is no longer held here, but g is out of scope, so we can't use
    // foo->baz or g->baz.
    return x;
}
```
