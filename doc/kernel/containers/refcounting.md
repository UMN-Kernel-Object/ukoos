# Reference counting

Many resources in the ukoOS kernel have dynamic enough ownership that reference counting is useful to determine if they're still in use and when they should be freed.

We have a helper type, `refcount_t`, to help manage these.
This is similar to the type with the same name in the Linux kernel, but we provide different helpers.

```c
#include <refcount.h>

/**
 * This type is some random example resource. It's always
 */
struct foo {
    /**
     * This field stores the actual reference count. This is an integral type
     * that is no wider than usize.
     */
    refcount_t refcount;

    /**
     * A list the value is in.
     */
    struct list_head all_foos;

    /**
     * Another list the value might be in (or the list might be self-linked).
     */
    struct list_head bars;
};

struct list_head all_foos;

void add_foos_not_in_a_bars_to_list(struct list_head *bars) {
    for (struct list_head *iter = all_foos.next; iter != all_foos;
            iter = iter->next) {
        struct foo *foo = container_of(iter, struct foo, all_foos);
        if (list_is_empty(foo->bars))
            continue;
        list_push(bars, &foo->bars);
    }
}
```
