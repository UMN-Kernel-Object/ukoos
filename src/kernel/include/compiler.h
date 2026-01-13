/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__COMPILER_H
#define UKO_OS_KERNEL__COMPILER_H 1

/**
 * Macros that expose compiler builtins.
 */

#define alignof _Alignof
#define atomic _Atomic
#define offsetof(type, member) __builtin_offsetof(type, member)
#define static_assert _Static_assert

typedef __builtin_va_list va_list;
#define va_start(ap) __builtin_va_start(ap, 0)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

#define likely(COND)                                                           \
  ({                                                                           \
    bool __likely_COND = (COND);                                               \
    __builtin_expect(__likely_COND, true);                                     \
  })
#define unlikely(COND)                                                         \
  ({                                                                           \
    bool __unlikely_COND = (COND);                                             \
    __builtin_expect(__unlikely_COND, false);                                  \
  })

#define ARRAY_SIZE(ARRAY)                                                      \
  ({                                                                           \
    static_assert(                                                             \
        !__builtin_types_compatible_p(typeof(ARRAY), typeof(&(ARRAY)[0])),     \
        #ARRAY " should be an array");                                         \
    sizeof(ARRAY) / sizeof((ARRAY)[0]);                                        \
  })

#define bswap(X)                                                               \
  (_Generic(X,                                                                 \
       u16: __builtin_bswap16,                                                 \
       u32: __builtin_bswap32,                                                 \
       u64: __builtin_bswap64)(X))

#define ckd_add(R, A, B) __builtin_add_overflow((A), (B), (R))
#define ckd_sub(R, A, B) __builtin_sub_overflow((A), (B), (R))
#define ckd_mul(R, A, B) __builtin_mul_overflow((A), (B), (R))

#define stdc_leading_zeros __builtin_stdc_leading_zeros
#define stdc_leading_ones __builtin_stdc_leading_ones
#define stdc_trailing_ones __builtin_stdc_trailing_ones
#define stdc_first_leading_zero __builtin_stdc_first_leading_zero
#define stdc_first_leading_one __builtin_stdc_first_leading_one
#define stdc_first_trailing_zero __builtin_stdc_first_trailing_zero
#define stdc_count_zeros __builtin_stdc_count_zeros
#define stdc_count_ones __builtin_stdc_count_ones

#if __has_builtin(__builtin_stdc_first_trailing_one)
#define stdc_first_trailing_one __builtin_stdc_first_trailing_one
#else
// https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html#index-_005f_005fbuiltin_005fstdc_005ffirst_005ftrailing_005fone
[[gnu::const]]
static inline __UINT8_TYPE__
__stdc_first_trailing_one_u8(__UINT8_TYPE__ value) {
  return (__UINT8_TYPE__)(__builtin_ctzg(value, -1) + 1);
}

[[gnu::const]]
static inline __UINT16_TYPE__
__stdc_first_trailing_one_u16(__UINT16_TYPE__ value) {
  return (__UINT16_TYPE__)(__builtin_ctzg(value, -1) + 1);
}

[[gnu::const]]
static inline __UINT32_TYPE__
__stdc_first_trailing_one_u32(__UINT32_TYPE__ value) {
  return (__UINT32_TYPE__)(__builtin_ctzg(value, -1) + 1);
}

[[gnu::const]]
static inline __UINT64_TYPE__
__stdc_first_trailing_one_u64(__UINT64_TYPE__ value) {
  return (__UINT64_TYPE__)(__builtin_ctzg(value, -1) + 1);
}

#define stdc_first_trailing_one(VALUE)                                         \
  (_Generic(VALUE,                                                             \
       u8: __stdc_first_trailing_one_u8,                                       \
       u16: __stdc_first_trailing_one_u16,                                     \
       u32: __stdc_first_trailing_one_u32,                                     \
       u64: __stdc_first_trailing_one_u64)(VALUE))
#endif

#if __has_builtin(__builtin_stdc_has_single_bit)
#define stdc_has_single_bit __builtin_stdc_has_single_bit
#else
// https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html#index-_005f_005fbuiltin_005fstdc_005fhas_005fsingle_005fbit
[[gnu::const]]
static inline __UINT8_TYPE__ __stdc_has_single_bit_u8(__UINT8_TYPE__ value) {
  return (bool)(__builtin_popcountg(value) == 1);
}

[[gnu::const]]
static inline __UINT16_TYPE__ __stdc_has_single_bit_u16(__UINT8_TYPE__ value) {
  return (bool)(__builtin_popcountg(value) == 1);
}

[[gnu::const]]
static inline __UINT32_TYPE__ __stdc_has_single_bit_u32(__UINT8_TYPE__ value) {
  return (bool)(__builtin_popcountg(value) == 1);
}

[[gnu::const]]
static inline __UINT64_TYPE__ __stdc_has_single_bit_u64(__UINT8_TYPE__ value) {
  return (bool)(__builtin_popcountg(value) == 1);
}

#define stdc_has_single_bit(VALUE)                                             \
  (_Generic(VALUE,                                                             \
       u8: __stdc_has_single_bit_u8,                                           \
       u16: __stdc_has_single_bit_u16,                                         \
       u32: __stdc_has_single_bit_u32,                                         \
       u64: __stdc_has_single_bit_u64)(VALUE))
#endif

#if __has_builtin(__builtin_stdc_bit_floor)
#define stdc_bit_floor __builtin_stdc_bit_floor
#else
// https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html#index-_005f_005fbuiltin_005fstdc_005fbit_005ffloor
[[gnu::const]]
static inline __UINT8_TYPE__ __stdc_bit_floor_u8(__UINT8_TYPE__ value) {
  return value == 0 ? (__UINT8_TYPE__)0
                    : (__UINT8_TYPE__)1 << (8 - 1 - __builtin_clzg(value));
}

[[gnu::const]]
static inline __UINT16_TYPE__ __stdc_bit_floor_u16(__UINT16_TYPE__ value) {
  return value == 0 ? (__UINT16_TYPE__)0
                    : (__UINT16_TYPE__)1 << (16 - 1 - __builtin_clzg(value));
}

[[gnu::const]]
static inline __UINT32_TYPE__ __stdc_bit_floor_u32(__UINT32_TYPE__ value) {
  return value == 0 ? (__UINT32_TYPE__)0
                    : (__UINT32_TYPE__)1 << (32 - 1 - __builtin_clzg(value));
}

[[gnu::const]]
static inline __UINT64_TYPE__ __stdc_bit_floor_u64(__UINT64_TYPE__ value) {
  return value == 0 ? (__UINT64_TYPE__)0
                    : (__UINT64_TYPE__)1 << (64 - 1 - __builtin_clzg(value));
}

#define stdc_bit_floor(VALUE)                                                  \
  (_Generic(VALUE,                                                             \
       u8: __stdc_bit_floor_u8,                                                \
       u16: __stdc_bit_floor_u16,                                              \
       u32: __stdc_bit_floor_u32,                                              \
       u64: __stdc_bit_floor_u64)(VALUE))
#endif

#if __has_builtin(__builtin_stdc_bit_width)
#define stdc_bit_width __builtin_stdc_bit_width
#else
// https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html#index-_005f_005fbuiltin_005fstdc_005fbit_005fwidth
[[gnu::const]]
static inline __UINT8_TYPE__ __stdc_bit_width_u8(__UINT8_TYPE__ value) {
  return (__UINT8_TYPE__)(8 - __builtin_clzg(value, 8));
}

[[gnu::const]]
static inline __UINT16_TYPE__ __stdc_bit_width_u16(__UINT16_TYPE__ value) {
  return (__UINT16_TYPE__)(16 - __builtin_clzg(value, 16));
}

[[gnu::const]]
static inline __UINT32_TYPE__ __stdc_bit_width_u32(__UINT32_TYPE__ value) {
  return (__UINT32_TYPE__)(32 - __builtin_clzg(value, 32));
}

[[gnu::const]]
static inline __UINT64_TYPE__ __stdc_bit_width_u64(__UINT64_TYPE__ value) {
  return (__UINT64_TYPE__)(64 - __builtin_clzg(value, 64));
}

#define stdc_bit_width(VALUE)                                                  \
  (_Generic(VALUE,                                                             \
       u8: __stdc_bit_width_u8,                                                \
       u16: __stdc_bit_width_u16,                                              \
       u32: __stdc_bit_width_u32,                                              \
       u64: __stdc_bit_width_u64)(VALUE))
#endif

#if __has_builtin(__builtin_stdc_trailing_zeros)
#define stdc_trailing_zeros __builtin_stdc_trailing_zeros
#else
[[gnu::const]]
static inline __UINT8_TYPE__ __stdc_trailing_zeros_u8(__UINT8_TYPE__ value) {
  return __builtin_ctzg((__UINT8_TYPE__)~value, -1) + 1U;
}

[[gnu::const]]
static inline __UINT16_TYPE__ __stdc_trailing_zeros_u16(__UINT16_TYPE__ value) {
  return __builtin_ctzg((__UINT16_TYPE__)~value, -1) + 1U;
}

[[gnu::const]]
static inline __UINT32_TYPE__ __stdc_trailing_zeros_u32(__UINT32_TYPE__ value) {
  return __builtin_ctzg((__UINT32_TYPE__)~value, -1) + 1U;
}

[[gnu::const]]
static inline __UINT64_TYPE__ __stdc_trailing_zeros_u64(__UINT64_TYPE__ value) {
  return __builtin_ctzg((__UINT64_TYPE__)~value, -1) + 1U;
}

[[gnu::const]]
static inline __SIZE_TYPE__ __stdc_trailing_zeros_usize(__SIZE_TYPE__ value) {
  return __builtin_ctzg((__SIZE_TYPE__)~value, -1) + 1U;
}

#define stdc_trailing_zeros(VALUE)                                             \
  (_Generic(VALUE,                                                             \
       u8: __stdc_trailing_zeros_u8,                                           \
       u16: __stdc_trailing_zeros_u16,                                         \
       u32: __stdc_trailing_zeros_u32,                                         \
       u64: __stdc_trailing_zeros_u64,                                         \
       usize: __stdc_trailing_zeros_usize)(VALUE))
#endif

#if __has_builtin(__builtin_stdc_bit_ceil)
#define stdc_bit_ceil __builtin_stdc_bit_ceil
#else
// https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html#index-_005f_005fbuiltin_005fstdc_005fbit_005fceil
[[gnu::const]]
static inline __UINT8_TYPE__ __stdc_bit_ceil_u8(__UINT8_TYPE__ value) {
  return value <= 1
             ? (__UINT8_TYPE__)1
             : (__UINT8_TYPE__)2
                   << (8 - 1 - __builtin_clzg((__UINT8_TYPE__)(value - 1)));
}

[[gnu::const]]
static inline __UINT16_TYPE__ __stdc_bit_ceil_u16(__UINT16_TYPE__ value) {
  return value <= 1
             ? (__UINT16_TYPE__)1
             : (__UINT16_TYPE__)2
                   << (16 - 1 - __builtin_clzg((__UINT16_TYPE__)(value - 1)));
}

[[gnu::const]]
static inline __UINT32_TYPE__ __stdc_bit_ceil_u32(__UINT32_TYPE__ value) {
  return value <= 1
             ? (__UINT32_TYPE__)1
             : (__UINT32_TYPE__)2
                   << (32 - 1 - __builtin_clzg((__UINT32_TYPE__)(value - 1)));
}

[[gnu::const]]
static inline __UINT64_TYPE__ __stdc_bit_ceil_u64(__UINT64_TYPE__ value) {
  return value <= 1
             ? (__UINT64_TYPE__)1
             : (__UINT64_TYPE__)2
                   << (64 - 1 - __builtin_clzg((__UINT64_TYPE__)(value - 1)));
}

[[gnu::const]]
static inline __SIZE_TYPE__ __stdc_bit_ceil_usize(__SIZE_TYPE__ value) {
  return value <= 1
             ? (__SIZE_TYPE__)1
             : (__SIZE_TYPE__)2 << (sizeof(__SIZE_TYPE__) * 8 - 1 -
                                    __builtin_clzg((__SIZE_TYPE__)(value - 1)));
}

#define stdc_bit_ceil(VALUE)                                                   \
  (_Generic(VALUE,                                                             \
       u8: __stdc_bit_ceil_u8,                                                 \
       u16: __stdc_bit_ceil_u16,                                               \
       u32: __stdc_bit_ceil_u32,                                               \
       u64: __stdc_bit_ceil_u64,                                               \
       usize: __stdc_bit_ceil_usize)(VALUE))
#endif

#if __has_builtin(__builtin_stdc_rotate_left)
#define stdc_rotate_left __builtin_stdc_rotate_left
#else
[[gnu::const]]
static inline __UINT8_TYPE__ __stdc_rotate_left_u8(__UINT8_TYPE__ value,
                                                   unsigned int count) {
  return (__UINT8_TYPE__)(value << count) | (value >> (8 - count));
}

[[gnu::const]]
static inline __UINT16_TYPE__ __stdc_rotate_left_u16(__UINT16_TYPE__ value,
                                                     unsigned int count) {
  return (__UINT16_TYPE__)(value << count) | (value >> (16 - count));
}

[[gnu::const]]
static inline __UINT32_TYPE__ __stdc_rotate_left_u32(__UINT32_TYPE__ value,
                                                     unsigned int count) {
  return (__UINT32_TYPE__)(value << count) | (value >> (32 - count));
}

[[gnu::const]]
static inline __UINT64_TYPE__ __stdc_rotate_left_u64(__UINT64_TYPE__ value,
                                                     unsigned int count) {
  return (__UINT64_TYPE__)(value << count) | (value >> (64 - count));
}

#define stdc_rotate_left(VALUE, COUNT)                                         \
  (_Generic(VALUE,                                                             \
       u8: __stdc_rotate_left_u8,                                              \
       u16: __stdc_rotate_left_u16,                                            \
       u32: __stdc_rotate_left_u32,                                            \
       u64: __stdc_rotate_left_u64)(VALUE, COUNT))
#endif

#if __has_builtin(__builtin_stdc_rotate_right)
#define stdc_rotate_right __builtin_stdc_rotate_right
#else
[[gnu::const]]
static inline __UINT8_TYPE__ __stdc_rotate_right_u8(__UINT8_TYPE__ value,
                                                    unsigned int count) {
  return (value >> count) | (__UINT8_TYPE__)(value << (8 - count));
}

[[gnu::const]]
static inline __UINT16_TYPE__ __stdc_rotate_right_u16(__UINT16_TYPE__ value,
                                                      unsigned int count) {
  return (value >> count) | (__UINT16_TYPE__)(value << (16 - count));
}

[[gnu::const]]
static inline __UINT32_TYPE__ __stdc_rotate_right_u32(__UINT32_TYPE__ value,
                                                      unsigned int count) {
  return (value >> count) | (__UINT32_TYPE__)(value << (32 - count));
}

[[gnu::const]]
static inline __UINT64_TYPE__ __stdc_rotate_right_u64(__UINT64_TYPE__ value,
                                                      unsigned int count) {
  return (value >> count) | (__UINT64_TYPE__)(value << (64 - count));
}

#define stdc_rotate_right(VALUE, COUNT)                                        \
  (_Generic(VALUE,                                                             \
       u8: __stdc_rotate_right_u8,                                             \
       u16: __stdc_rotate_right_u16,                                           \
       u32: __stdc_rotate_right_u32,                                           \
       u64: __stdc_rotate_right_u64)(VALUE, COUNT))
#endif

/**
 * String functions. These don't _look_ builtin (hey, we're defining them here
 * ourselves!), but the compiler's optimizer treats them as builtins, and can do
 * certain optimizations that it wouldn't otherwise be able to do.
 */

[[gnu::access(write_only, 1), gnu::nonnull(1)]]
void bzero(void *dst, __SIZE_TYPE__ len);

[[gnu::access(write_only, 1), gnu::nonnull(1)]]
void explicit_bzero(void *dst, __SIZE_TYPE__ len);

[[gnu::access(write_only, 1), gnu::nonnull(1, 2)]]
void *memcpy(void *restrict dst, const void *restrict src, __SIZE_TYPE__ len);

[[gnu::access(write_only, 1), gnu::nonnull(1)]]
void *memset(void *restrict dst, int byte, __SIZE_TYPE__ len);

[[gnu::nonnull(1, 2), gnu::pure]]
int memcmp(const void *s1, const void *s2, __SIZE_TYPE__ n);

[[gnu::nonnull(1, 2), gnu::pure]]
int strcmp(const char *s1, const char *s2);

[[gnu::nonnull(1), gnu::pure]]
__SIZE_TYPE__ strlen(const char *s);

[[gnu::nonnull(1), gnu::pure]]
__SIZE_TYPE__ strnlen(const char *s, __SIZE_TYPE__ max_len);

#endif // UKO_OS_KERNEL__COMPILER_H
