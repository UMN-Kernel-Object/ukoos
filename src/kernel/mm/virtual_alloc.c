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
 *
 * If you're not already familiar with treaps, there's a Julia Evans piece about
 * them: https://jvns.ca/blog/2017/09/09/data-structure--the-treap-/
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
 * Performs a tree rotation. Visually, this transformation is:
 *
 *         P               P
 *         │               │
 *       ╭ D ╮           ╭ B ╮
 *     ╭ B ╮ E    <=>    A ╭ D ╮
 *     A   C               C   E
 *
 * When the `which_child` argument is `LEFT_CHILD`, the `vma` argument is the
 * `D` node, and the rotation goes from the left figure to the right figure.
 *
 * When the `which_child` argument is `RIGHT_CHILD`, the `vma` argument is the
 * `B` node, and the rotation goes from the right figure to the left figure.
 */
static void treap_rotate(struct vma *vma, enum which_treap which,
                         enum which_child which_child) {
  struct vma *d_b = vma;
  struct treap_head *d_b_treap = &d_b->treaps[which];
  struct vma *b_d = d_b_treap->children[which_child];
  struct treap_head *b_d_treap = &b_d->treaps[which];
  struct vma *p = d_b_treap->parent;

  struct vma *c = b_d_treap->children[!which_child];
  d_b_treap->children[which_child] = c;
  if (c) {
    struct treap_head *c_treap = &c->treaps[which];
    c_treap->parent = d_b;
    c_treap->which_child = which_child;
  }

  b_d_treap->children[!which_child] = d_b;
  b_d_treap->parent = p;
  b_d_treap->which_child = d_b_treap->which_child;

  d_b_treap->parent = b_d;
  d_b_treap->which_child = !which_child;

  if (p) {
    p->treaps[which].children[b_d_treap->which_child] = b_d;
  } else {
    b_d->allocator->roots[which] = b_d;
  }
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

    // Rotate down the parent.
    treap_rotate(parent, which, which_child);

    // Update the parent.
    parent = treap->parent;
    if (!parent)
      break;
    parent_treap = &parent->treaps[which];
    which_child = treap->which_child;
  }
}

/**
 * Removes a VMA from a treap.
 */
static void treap_remove(struct vma *vma, enum which_treap which) {
  struct treap_head *treap = &vma->treaps[which];
  assert(treap->priority);

  // Break invariants(!) -- set our priority to zero. Then, perform tree
  // rotations until we're a leaf.
  treap->priority = 0;
  for (;;) {
    enum which_child which_child;
    if (treap->children[LEFT_CHILD]) {
      which_child = LEFT_CHILD;
    } else if (treap->children[RIGHT_CHILD]) {
      which_child = RIGHT_CHILD;
    } else {
      break;
    }

    treap_rotate(vma, which, which_child);
  }

  // Once we're a leaf, we can simply remove ourselves without needing to
  // rebalance.
  if (treap->parent) {
    // We're not the root.
    assert(treap->parent->treaps[which].children[treap->which_child] == vma);
    treap->parent->treaps[which].children[treap->which_child] = nullptr;
  } else {
    // We're the root.
    assert(vma->allocator->roots[which] == vma);
    vma->allocator->roots[which] = nullptr;
  }

  // Reinitialize ourselves.
  treap_init(treap, treap->value);
}

/**
 * Returns the size of a VMA.
 */
static usize vma_size(const struct vma *vma) {
  u32 size_compressed = vma->treaps[BY_SIZE].value;
  return ((usize)size_compressed + 1) << 12;
}

/**
 * Writes the bounds of a VMA to out_lo and out_hi.
 */
void vma_bounds(const struct vma *vma, uaddr *out_lo, uaddr *out_hi) {
  uaddr base_address = vma->allocator->base_address;
  u32 addr_compressed = vma->treaps[BY_ADDR].value;
  uaddr addr = base_address + ((uaddr)addr_compressed << 12);
  usize size = vma_size(vma);

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

static struct vma *vma_find(struct vma_allocator *allocator, uaddr addr) {
  struct vma *vma = allocator->roots[BY_ADDR];
  while (vma) {
    uaddr start, end;
    vma_bounds(vma, &start, &end);
    if (addr < start)
      vma = vma->treaps[BY_ADDR].children[LEFT_CHILD];
    else if (end <= addr)
      vma = vma->treaps[BY_ADDR].children[RIGHT_CHILD];
    else
      break;
  }
  return vma;
}

// TODO: There's a circularity here we need to break. When we split a VMA, we
// need to allocate memory. If we need to grow the heap while doing so, the
// allocator needs to allocate virtual memory.
struct vma *vma_alloc(struct vma_allocator *allocator, usize num_pages) {
  assert(0 < num_pages && num_pages <= (1 << 26));
  usize wanted_size = num_pages << 12;

  // Find the smallest VMA that fits.
  struct vma *vma = allocator->roots[BY_SIZE];
  bool needs_to_split;
  while (vma) {
    usize size = vma_size(vma);
    if (size < wanted_size) {
      vma = vma->treaps[BY_SIZE].children[RIGHT_CHILD];
    } else if (size > wanted_size) {
      // Check if we _can_ go smaller; if not, return this one.
      struct vma *smaller_vma = vma->treaps[BY_SIZE].children[LEFT_CHILD];
      if (smaller_vma) {
        vma = smaller_vma;
      } else {
        needs_to_split = true;
        break;
      }
    } else {
      needs_to_split = false;
      break;
    }
  }

  // If we couldn't find a VMA that fits, return an OOM.
  if (!vma)
    return nullptr;

  if (needs_to_split) {
    // If we need to split the VMA in two to get something that fits, do that.
    struct vma *new_vma = alloc(sizeof(struct vma));
    if (!new_vma)
      return nullptr;

    // We'll shrink the VMA we already had to fit the wanted size, and put the
    // remaining range in the new VMA. We get the current bounds, so we can size
    // the new VMA to fit.
    uaddr old_lo, old_hi, new_lo, new_hi;
    vma_bounds(vma, &old_lo, &old_hi);

    // Remove the VMA from the free treap and resize it, computing the new
    // bounds.
    treap_remove(vma, BY_SIZE);
    vma->treaps[BY_SIZE].value = (u32)((wanted_size >> 12) - 1);
    vma_bounds(vma, &new_lo, &new_hi);

    // Now we can initialize the new VMA and insert it into the list and both
    // treaps.
    vma_init(allocator, new_hi, old_hi, new_vma);
    list_unshift(&vma->list_head, &new_vma->list_head);
    treap_insert(new_vma, BY_ADDR);
    treap_insert(new_vma, BY_SIZE);

    // The old VMA is now marked as used, and the new VMA is marked as free. The
    // same range of addresses are be covered.
    return vma;
  } else {
    // Otherwise, just remove the VMA from the free treap and return it.
    treap_remove(vma, BY_SIZE);
    return vma;
  }
}

struct vma *vma_alloc_by_addr(struct vma_allocator *allocator, uaddr lo,
                              uaddr hi) {
  assert((lo & 0xfff) == 0 && (hi & 0xfff) == 0);
  assert(lo < hi);

  struct vma *vma = vma_find(allocator, lo);
  if (!vma)
    return nullptr;

  // If the VMA isn't free, we can't allocate.
  if (!is_vma_free(vma))
    return nullptr;

  // If the same VMA doesn't contain the highest address, there must be another
  // allocation inside the range.
  uaddr vma_lo, vma_hi;
  vma_bounds(vma, &vma_lo, &vma_hi);
  if (vma_hi < hi)
    return nullptr;

  // Now, we want to split the VMA we found to contain the allocation.
  //
  // The simplest thing to do is to remove the VMA from the treaps, split it,
  // and reinsert the split-out parts. We keep it in the list, so that we can
  // avoid re-traversing it later.
  //
  // Before we do, we allocate memory for the new VMAs to be stored, so we don't
  // have to "undo" work if allocation fails later.
  struct vma *new_vma_lo = nullptr, *new_vma_hi = nullptr;
  if (vma_lo != lo)
    if (!(new_vma_lo = alloc(sizeof(struct vma))))
      goto fail;
  if (vma_hi != hi)
    if (!(new_vma_hi = alloc(sizeof(struct vma))))
      goto fail;

  // Save a pointer to the previous VMA (or the allocator), so we can insert all
  // the VMAs into the list without searching.
  struct list_head *prev = vma->list_head.prev;

  // Remove the VMA. This breaks invariants (the allocator may now be empty),
  // but we restore them later.
  list_remove(&vma->list_head);
  treap_remove(vma, BY_ADDR);
  treap_remove(vma, BY_SIZE);

  // Initialize the VMAs and insert them into the list and treaps.
  if (new_vma_hi) {
    vma_init(allocator, hi, vma_hi, new_vma_hi);
    list_unshift(prev, &new_vma_hi->list_head);
    treap_insert(new_vma_hi, BY_ADDR);
    treap_insert(new_vma_hi, BY_SIZE);
  }
  vma_init(allocator, lo, hi, vma);
  list_unshift(prev, &vma->list_head);
  treap_insert(vma, BY_ADDR);
  if (new_vma_lo) {
    vma_init(allocator, vma_lo, lo, new_vma_lo);
    list_unshift(prev, &new_vma_lo->list_head);
    treap_insert(new_vma_lo, BY_ADDR);
    treap_insert(new_vma_lo, BY_SIZE);
  }

  // Return the VMA.
  return vma;

fail:
  free(new_vma_lo);
  free(new_vma_hi);
  return nullptr;
}

void vma_free(struct vma *vma) {
  uaddr hi, lo;

  if (!vma)
    return;

  vma_bounds(vma, &hi, &lo);
  TODO("vma_free {uaddr}-{uaddr}", hi, lo);
}
