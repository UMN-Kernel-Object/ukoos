# Lists

Many resources in the ukoOS kernel need to be in some registry.
For example, devices of a certain device class (e.g. UARTs) generally need to be in some list, so that the kernel can iterate through all UARTs.
Lists (`struct list_head`, after the same structure in Linux) allow for this in ukoOS.
See `src/kernel/include/list.h` for the actual list API.

ukoOS lists are doubly-linked circular *intrusive* linked lists; that is, rather than the list containing the element in some way (whether by having a pointer that points to it or by including it in each link's memory allocation), elements contain the link.

For example, let's say we had a list of RGB colors and names.
We might write that data structure as:

```c
struct named_color {
    struct list_head list;
    const char *name;
    u8 r, g, b;
};
```

This might seem strange, but it has one big advantage over a container that owns its data -- elements can belong to multiple lists.
For example, pretend we had both an `all_colors` list and a `favorite_colors` list:

```c
struct named_color {
    struct list_head all_colors;
    struct list_head favorite_colors;
    const char *name;
    u8 r, g, b;
};
```

This lets the color be in both lists at once.

Lists are intended to have a single owning node / sentinel node.
This is **not** embedded in the structure like other nodes are, but stands alone, often as a global:

```c
struct named_color {
    struct list_head all_colors;
    struct list_head favorite_colors;
    const char *name;
    u8 r, g, b;
};

struct list_head all_colors = LIST_INIT(all_colors);
struct list_head favorite_colors = LIST_INIT(favorite_colors);
```

Since the lists are circular, we can use this sentinel node to know when we've reached the end of the list.

We can also use this to create a tree:

```c
struct tree_node {
    /**
     * The parent of this node.
     */
    struct tree_node *parent;

    /**
     * This node's link in its parent's children.
     */
    struct list_head list;

    /**
     * This node's children.
     */
    struct list_head children;
};
```

The `container_of` macro can be used to go from a `struct list_head *` to a `struct tree_node *`:

```c
struct tree_node node = { /* ... */ };

struct list_head *ptr1 = &node.list;
struct tree_node *node1 = container_of(ptr1, struct tree_node, list);
assert(node1 == &node);

struct list_head *ptr2 = &node.children;
struct tree_node *node2 = container_of(ptr2, struct tree_node, children);
assert(node2 == &node);
```
