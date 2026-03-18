/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

constexpr i8 I8_MAX = (i8)0x7f;
constexpr i8 I8_MIN = (i8)0x80;
constexpr i16 I16_MAX = (i16)0x7fff;
constexpr i16 I16_MIN = (i16)0x8000;
constexpr i32 I32_MAX = (i32)0x7fffffffUL;
constexpr i32 I32_MIN = (i32)0x80000000UL;
constexpr i64 I64_MAX = (i64)0x7fffffffffffffffULL;
constexpr i64 I64_MIN = (i64)0x8000000000000000ULL;
constexpr u8 U8_MAX = 0xff;
constexpr u16 U16_MAX = 0xffff;
constexpr u32 U32_MAX = 0xffffffffUL;
constexpr u64 U64_MAX = 0xffffffffffffffffULL;

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
 * The type of null pointers.
 */

typedef typeof(nullptr) nullptr_t;

/**
 * Strict provenance functions. See [0] for more details, but these are
 * basically APIs for doing "funny things" to pointers that compile better on
 * architectures like CHERI or Fil-C than arbitrary int2ptr casts.
 *
 * At a high level, you can imagine pointers as actually being a pair of
 * `(provenance, address)`. Casting ptr2int returns the address part of the
 * pointer. Strict provenance gives APIs to build pointers from that pair,
 * instead of relying on int2ptr casts doing "the right thing" to get the
 * provenance part.
 *
 * The current C standard does have a notion of "the right thing"
 * (exposed addresses with user disambiguation; see PNVI-ae-udi in [1]), but it
 * is complicated and does not compile well to CHERI or Fil-C.
 *
 * Strict provenance instead restricts the set of operations to only those that
 * are compatible with provenance needing to be explicitly passed around. This
 * simplifies the formal model, and is what's literally happening in CHERI and
 * Fil-C.
 *
 * [0]: https://github.com/rust-lang/rust/issues/95228
 * [1]: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2364.pdf
 */

static inline uaddr _addr_of_ptr(const void *ptr) {
  // This is written rather awkwardly to avoid exposing `ptr`'s provenance.
  uaddr addr;
  memcpy(&addr, &ptr, sizeof(uaddr));
  return addr;
}

static inline uaddr _addr_of_ptr_uptr(uptr ptr) {
  // This is written rather awkwardly to avoid exposing `ptr`'s provenance.
  uaddr addr;
  memcpy(&addr, &ptr, sizeof(uaddr));
  return addr;
}

/**
 * Returns the address part of a pointer.
 */
#define addr_of_ptr(PTR)                                                       \
  ({                                                                           \
    const auto __addr_of_ptr_PTR = (PTR);                                      \
    constexpr auto __addr_of_ptr_PTR_type =                                    \
        __builtin_classify_type(__addr_of_ptr_PTR);                            \
    const auto __addr_of_ptr_PTR_or_null = __builtin_choose_expr(              \
        __addr_of_ptr_PTR_type == __builtin_classify_type(void *),             \
        __addr_of_ptr_PTR, (void **)nullptr);                                  \
    const auto __addr_of_ptr_func = _Generic(__addr_of_ptr_PTR,                \
        typeof(*__addr_of_ptr_PTR_or_null) *: _addr_of_ptr,                    \
        nullptr_t: _addr_of_ptr,                                               \
        uptr: _addr_of_ptr_uptr);                                              \
    __addr_of_ptr_func(__addr_of_ptr_PTR);                                     \
  })

static inline void *_ptr_with_addr(void *ptr, uaddr addr) {
  return ptr + (addr - addr_of_ptr(ptr));
}

static inline const void *_ptr_with_addr_const(const void *ptr, uaddr addr) {
  return ptr + (addr - addr_of_ptr(ptr));
}

static inline const uptr _ptr_with_addr_uptr(const uptr ptr_uptr, uaddr addr) {
  // This is written rather awkwardly to avoid exposing `ptr_uptr`'s provenance.
  void *ptr_ptr;
  memcpy(&ptr_ptr, &ptr_uptr, sizeof(uptr));
  void *out_ptr = _ptr_with_addr(ptr_ptr, addr);
  uptr out_uptr;
  memcpy(&out_uptr, &out_ptr, sizeof(uptr));
  return out_uptr;
}

/**
 * Returns a pointer with the same provenance as `PTR` and the address `ADDR`.
 *
 * Note it is UB if `ADDR` is zero and `PTR` is not `nullptr`.
 */
#define ptr_with_addr(PTR, ADDR)                                               \
  ({                                                                           \
    const auto __ptr_with_addr_PTR = (PTR);                                    \
    const auto __ptr_with_addr_ADDR = (ADDR);                                  \
    constexpr auto __ptr_with_addr_PTR_type =                                  \
        __builtin_classify_type(__ptr_with_addr_PTR);                          \
    const auto __ptr_with_addr_PTR_or_null = __builtin_choose_expr(            \
        __ptr_with_addr_PTR_type == __builtin_classify_type(void *),           \
        __ptr_with_addr_PTR, (void **)nullptr);                                \
    const auto __ptr_with_addr_func = _Generic(__ptr_with_addr_PTR,            \
        typeof(*__ptr_with_addr_PTR_or_null) *: _ptr_with_addr,                \
        const typeof(*__ptr_with_addr_PTR_or_null) *: _ptr_with_addr_const,    \
        nullptr_t: _ptr_with_addr,                                             \
        uptr: _ptr_with_addr_uptr);                                            \
    __ptr_with_addr_func(__ptr_with_addr_PTR, __ptr_with_addr_ADDR);           \
  })

/**
 * Returns a pointer without a valid provenance and the address `addr`. This
 * pointer can have `addr_of_ptr` called on it, but most other operations are
 * UB (e.g. dereferencing the pointer or comparing it to another pointer).
 */
static inline void *ptr_without_provenance(uaddr addr) {
  return ptr_with_addr(nullptr, addr);
}

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

#if __SIZEOF_SIZE_T__ == 8
constexpr isize ISIZE_MAX = I64_MAX;
constexpr isize ISIZE_MIN = I64_MIN;
constexpr usize USIZE_MAX = U64_MAX;
#else
#error TODO: Unsupported target
#endif

#endif // UKO_OS_KERNEL__TYPES_H
