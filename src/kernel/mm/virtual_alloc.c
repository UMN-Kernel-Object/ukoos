/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <mm/virtual_alloc.h>
#include <print.h>
#include <random.h>

/**
 * The virtual memory allocator.
 */

/**
 * An enum to describe which treap is being accessed.
 */
enum which_treap {
  BY_ADDR = 0,
  BY_SIZE = 1,
};

/**
 * An instance of the virtual memory allocator. The allocator stores a
 * non-empty sequence of VMAs, in a few forms.
 *
 * - All VMAs are stored in an intrusive doubly linked list, sorted by their
 *   start address. This gives VMAs easy access to their siblings, letting them
 *   be merged when freed.
 * - All VMAs are stored in a treap, sorted by their start address. This allows
 *   efficiently finding a VMA by address.
 * - All free VMAs are stored in a treap, sorted by their size. This allows
 *   efficiently allocating a free VMA.
 *
 * A few invariants hold:
 *
 * - No two VMAs in the allocator overlap.
 * - All operations preserve the total area covered by VMAs.
 */
struct vma_allocator {
  /**
   * The `list_head` for the list of all VMAs.
   */
  struct list_head vmas;

  /**
   * The address the allocator starts at. This is used to compress the bounds of
   * the `value` field in `treap_head`, allowing for better alignment.
   */
  uaddr base_address;

  /**
   * The root nodes of the treaps. Indexed with `enum which_treap`.
   */
  struct vma *roots[2];
};

/**
 * An enum to describe whether a node is a left-child or right-child.
 */
enum which_child {
  LEFT_CHILD,
  RIGHT_CHILD,
};

/**
 * The treap fields, which are most of the data of a VMA.
 */
struct treap_head {
  /**
   * The value, which the binary search tree is sorted by.
   *
   * - For `BY_ADDR`, this is the offset from the allocator's `base_address`, in
   *   pages.
   * - For `BY_SIZE`, this is 1 less than the size of the VMA, in pages.
   */
  u32 value;

  /**
   * A flag for whether this node is a left child or a right child.
   */
  enum which_child which_child : 1;

  /**
   * The priority, which the heap is ordered by. This is non-zero when the VMA
   * is in the treap.
   */
  u32 priority : 31;

  /**
   * The parent node, or nullptr if this is the root.
   */
  struct vma *parent;

  /**
   * The child nodes. Indexed by `enum which_child`.
   */
  struct vma *children[2];
};

struct vma {
  /**
   * The `list_head` for this VMA's entry in `allocator->vmas`.
   *
   * This list is sorted by start address.
   */
  struct list_head list_head;

  /**
   * The allocator this VMA belongs to.
   */
  struct vma_allocator *allocator;

  /**
   * The `treap_head`s for this VMA's entry in the treaps. Indexed with
   * `enum which_treap`.
   */
  struct treap_head treaps[2];

  /**
   * Padding to align the VMA to two cachelines. We should find a better use for
   * this; probably generalizing one or both treaps to a B-tree.
   *
   * We'll also need a field for indicating what a VMA is backed by, for e.g.
   * memory-mapped VMAs that we're doing demand paging for.
   */
  u64 padding[5];
};

// Check that the size of a VMA is cacheline-aligned.
static_assert(sizeof(struct vma) == 128);
static_assert(sizeof(((struct vma *)nullptr)->padding) < 64,
              "Remove the padding from struct vma; it's small enough to no "
              "longer need it");

/**
 * Initializes a `struct treap_head`.
 */
static void treap_init(struct treap_head *treap, u32 value) {
  treap->value = value;
  treap->which_child = LEFT_CHILD;
  treap->priority = 0;
  treap->parent = nullptr;
  treap->children[LEFT_CHILD] = nullptr;
  treap->children[RIGHT_CHILD] = nullptr;
}

/**
 * Inserts a VMA into a treap.
 */
static void treap_insert(struct vma *vma, enum which_treap which) {
  struct treap_head *treap = &vma->treaps[which];
  assert(treap->which_child == LEFT_CHILD && !treap->priority &&
             !treap->parent && !treap->children[LEFT_CHILD] &&
             !treap->children[RIGHT_CHILD],
         "VMA must not already be in a treap");

  // Assign a priority.
  u32 priority;
  do {
    priority = random_u32();
  } while (unlikely(!priority));
  treap->which_child = LEFT_CHILD;
  treap->priority = priority & 0x7fffffff;

  // If the treap was empty, simply become the new root.
  struct vma *parent = vma->allocator->roots[which];
  if (!parent) {
    vma->allocator->roots[which] = vma;
    return;
  }

  // Otherwise, insert the VMA at the right location to maintain the binary
  // search tree property, ignoring the heap property.
  struct treap_head *parent_treap;
  enum which_child which_child;
  for (;;) {
    parent_treap = &parent->treaps[which];
    u32 parent_value = parent_treap->value;
    which_child = (treap->value <= parent_value) ? LEFT_CHILD : RIGHT_CHILD;
    if (!parent_treap->children[which_child])
      break;
    parent = parent_treap->children[which_child];
  }
  parent_treap->children[which_child] = vma;
  treap->which_child = which_child;
  treap->parent = parent;

  // Now, walk up the tree, restoring the heap property while maintaining the
  // binary search tree property.
  for (;;) {
    // If the heap property is already true locally, it's true globally.
    if (parent_treap->priority >= treap->priority)
      break;

    // Otherwise, rotate the tree to fix up the property locally. In the
    // LEFT_CHILD case (w.l.o.g.), this looks like:
    //
    //       ╭ P ╮          ╭ T ╮
    //     ╭ T ╮ C    =>    A ╭ P ╮
    //     A   B              B   C
    //
    // This maintains the binary search tree invariant; in both cases, the
    // invariant is `A <= T <= B <= P <= C`.
    struct vma *b = treap->children[!which_child];

    treap->which_child = parent_treap->which_child;
    treap->parent = parent_treap->parent;

    parent_treap->children[which_child] = b;
    if (b) {
      struct treap_head *b_treap = &b->treaps[which];
      b_treap->which_child = which_child;
      b_treap->parent = parent;
    }

    treap->children[!which_child] = parent;
    parent_treap->which_child = !which_child;
    parent_treap->parent = vma;

    // Update the parent.
    parent = treap->parent;
    if (!parent)
      break;
    parent_treap = &parent->treaps[which];
    which_child = treap->which_child;
  }
}

/**
 * Computes the bounds of a VMA.
 */
static void vma_bounds(const struct vma *vma, uaddr *out_lo, uaddr *out_hi) {
  uaddr base_address = vma->allocator->base_address;
  u32 addr_compressed = vma->treaps[BY_ADDR].value,
      size_compressed = vma->treaps[BY_SIZE].value;

  uaddr addr = base_address + ((uaddr)addr_compressed << 12);
  usize size = ((usize)size_compressed + 1) << 12;

  if (out_lo)
    *out_lo = addr;
  if (out_hi)
    *out_hi = addr + size;
}

static bool is_vma_free(const struct vma *vma) {
  return vma->treaps[BY_SIZE].priority != 0;
}

static void vma_init(struct vma_allocator *allocator, uaddr lo, uaddr hi,
                     struct vma *out) {
  assert((lo & 0xfff) == 0 && (hi & 0xfff) == 0);
  assert(lo < hi);
  assert(allocator->base_address <= lo);

  uaddr addr = lo - allocator->base_address;
  usize size = hi - lo;
  assert((addr >> 12) <= U32_MAX);
  assert(((size >> 12) - 1) <= U32_MAX);

  u32 addr_compressed = (u32)(addr >> 12);
  u32 size_compressed = (u32)((size >> 12) - 1);

  out->list_head = LIST_INIT(out->list_head);
  out->allocator = allocator;
  treap_init(&out->treaps[BY_ADDR], addr_compressed);
  treap_init(&out->treaps[BY_SIZE], size_compressed);
  bzero(out->padding, sizeof(out->padding));
}

struct vma_allocator *vma_allocator_new(uaddr lo, uaddr hi) {
  assert((lo & 0xfff) == 0 && (hi & 0xfff) == 0);
  assert(lo < hi);
  assert(hi <= 0x0000004000000000 || 0xffffffc000000000 <= lo);

  // Ensure the size of any VMA (measured in pages) can be represented in 32
  // bits.
  assert((hi - lo) <= ((usize)1 << (32 + 12)));

  struct vma_allocator *allocator = alloc(sizeof(struct vma_allocator));
  struct vma *vma = alloc(sizeof(struct vma));
  if (!allocator || !vma)
    goto fail;

  *allocator = (struct vma_allocator){
      .vmas = LIST_INIT(allocator->vmas),
      .base_address = lo,
      .roots = {nullptr, nullptr}, // Initialized below.
  };
  vma_init(allocator, lo, hi, vma);
  list_push(&allocator->vmas, &vma->list_head);
  treap_insert(vma, BY_ADDR);
  treap_insert(vma, BY_SIZE);
  return allocator;

fail:
  free(vma);
  free(allocator);
  return nullptr;
}

void vma_allocator_print(struct vma_allocator *allocator) {
  assert(allocator && !list_is_empty(&allocator->vmas));

  uaddr alloc_start, alloc_end;
  vma_bounds(container_of(allocator->vmas.next, struct vma, list_head),
             &alloc_start, nullptr);
  vma_bounds(container_of(allocator->vmas.prev, struct vma, list_head), nullptr,
             &alloc_end);

  print("VMA allocator ({uptr}), from range {uaddr} to {uaddr} ({usize} GiB):",
        allocator, alloc_start, alloc_end, (alloc_end - alloc_start) >> 30);

  for (struct list_head *child = allocator->vmas.next;
       child != &allocator->vmas; child = child->next) {
    struct vma *vma = container_of(child, struct vma, list_head);
    uaddr start, end;
    vma_bounds(vma, &start, &end);
    print("  {uaddr}-{uaddr}: {cstr}", start, end,
          is_vma_free(vma) ? "FREE" : "USED");
  }
}
