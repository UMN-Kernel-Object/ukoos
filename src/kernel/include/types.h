#ifndef UKO_OS_KERNEL__TYPES_H
#define UKO_OS_KERNEL__TYPES_H 1

/**
 * Common types used in almost every file of the kernel. When possible, prefer
 * to declare structs here (e.g. `struct foo;`) rather than defining them (e.g.
 * `struct foo { int bar; };`)
 */

#include <compiler.h>

/**
 * Fixed-size types.
 */

typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;
typedef signed long long i64;
typedef unsigned long long u64;

static_assert(sizeof(i8) == 1, "incorrect size for i8");
static_assert(alignof(i8) == 1, "incorrect alignment for i8");
static_assert(sizeof(u8) == 1, "incorrect size for u8");
static_assert(alignof(u8) == 1, "incorrect alignment for u8");
static_assert(sizeof(i16) == 2, "incorrect size for i16");
static_assert(alignof(i16) == 2, "incorrect alignment for i16");
static_assert(sizeof(u16) == 2, "incorrect size for u16");
static_assert(alignof(u16) == 2, "incorrect alignment for u16");
static_assert(sizeof(i32) == 4, "incorrect size for i32");
static_assert(alignof(i32) == 4, "incorrect alignment for i32");
static_assert(sizeof(u32) == 4, "incorrect size for u32");
static_assert(alignof(u32) == 4, "incorrect alignment for u32");
static_assert(sizeof(i64) == 8, "incorrect size for i64");
static_assert(alignof(i64) == 8, "incorrect alignment for i64");
static_assert(sizeof(u64) == 8, "incorrect size for u64");
static_assert(alignof(u64) == 8, "incorrect alignment for u64");

/**
 * Pointer-sized types.
 */

typedef signed long iptr;
typedef unsigned long uptr;

static_assert(sizeof(iptr) == sizeof(void *), "incorrect size for iptr");
static_assert(alignof(iptr) == alignof(void *), "incorrect alignment for iptr");
static_assert(sizeof(uptr) == sizeof(void *), "incorrect size for uptr");
static_assert(alignof(uptr) == alignof(void *), "incorrect alignment for uptr");

/**
 * Address-sized types.
 */

typedef signed long iaddr;
typedef unsigned long uaddr;

static_assert(sizeof(iaddr) == __SIZEOF_SIZE_T__, "incorrect size for iaddr");
static_assert(alignof(iaddr) == __SIZEOF_SIZE_T__,
              "incorrect alignment for iaddr");
static_assert(sizeof(uaddr) == __SIZEOF_SIZE_T__, "incorrect size for uaddr");
static_assert(alignof(uaddr) == __SIZEOF_SIZE_T__,
              "incorrect alignment for uaddr");

/**
 * A physical address.
 */
typedef struct {
  unsigned long offset : 12;
  unsigned long ppn : 44;
  unsigned long rsvd : 8;
} paddr;

static_assert(sizeof(iaddr) == __SIZEOF_PTRDIFF_T__,
              "incorrect size for iaddr");
static_assert(alignof(iaddr) == __SIZEOF_PTRDIFF_T__,
              "incorrect alignment for iaddr");
static_assert(sizeof(uaddr) == __SIZEOF_PTRDIFF_T__,
              "incorrect size for uaddr");
static_assert(alignof(uaddr) == __SIZEOF_PTRDIFF_T__,
              "incorrect alignment for uaddr");
static_assert(sizeof(paddr) == __SIZEOF_PTRDIFF_T__,
              "incorrect size for paddr");
static_assert(alignof(paddr) == __SIZEOF_PTRDIFF_T__,
              "incorrect alignment for paddr");

/**
 * Size types.
 */

typedef signed long isize;
typedef unsigned long usize;

static_assert(sizeof(isize) == __SIZEOF_SIZE_T__, "incorrect size for isize");
static_assert(alignof(isize) == __SIZEOF_SIZE_T__,
              "incorrect alignment for isize");
static_assert(sizeof(usize) == __SIZEOF_SIZE_T__, "incorrect size for usize");
static_assert(alignof(usize) == __SIZEOF_SIZE_T__,
              "incorrect alignment for usize");

#endif // UKO_OS_KERNEL__TYPES_H
