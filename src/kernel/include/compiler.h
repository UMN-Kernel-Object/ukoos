#ifndef UKO_OS_KERNEL__COMPILER_H
#define UKO_OS_KERNEL__COMPILER_H 1

/**
 * Macros that expose compiler builtins.
 */

#define alignof _Alignof
#define offsetof(type, member) __builtin_offsetof(type, member)
#define static_assert _Static_assert

typedef __builtin_va_list va_list;
#define va_start(ap) __builtin_va_start(ap, 0)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

#define bswap(X)                                                               \
  (_Generic(X,                                                                 \
       u16: __builtin_bswap16,                                                 \
       u32: __builtin_bswap32,                                                 \
       u64: __builtin_bswap64)(X))

#define stdc_leading_zeros __builtin_stdc_leading_zeros
#define stdc_leading_ones __builtin_stdc_leading_ones
#define stdc_trailing_zeros __builtin_stdc_trailing_zeros
#define stdc_trailing_ones __builtin_stdc_trailing_ones
#define stdc_first_leading_zero __builtin_stdc_first_leading_zero
#define stdc_first_leading_one __builtin_stdc_first_leading_one
#define stdc_first_trailing_zero __builtin_stdc_first_trailing_zero
#define stdc_first_trailing_one __builtin_stdc_first_trailing_one
#define stdc_count_zeros __builtin_stdc_count_zeros
#define stdc_count_ones __builtin_stdc_count_ones
#define stdc_has_single_bit __builtin_stdc_has_single_bit
#define stdc_bit_width __builtin_stdc_bit_width
#define stdc_bit_floor __builtin_stdc_bit_floor
#define stdc_bit_ceil __builtin_stdc_bit_ceil

#if __has_builtin(__builtin_stdc_rotate_left)
#define stdc_rotate_left __builtin_stdc_rotate_left
#else
[[unsequenced]]
static inline __UINT8_TYPE__ __stdc_rotate_left_u8(__UINT8_TYPE__ value,
                                                   unsigned int count) {
  return (__UINT8_TYPE__)(value << count) | (value >> (8 - count));
}

[[unsequenced]]
static inline __UINT16_TYPE__ __stdc_rotate_left_u16(__UINT16_TYPE__ value,
                                                     unsigned int count) {
  return (__UINT16_TYPE__)(value << count) | (value >> (16 - count));
}

[[unsequenced]]
static inline __UINT32_TYPE__ __stdc_rotate_left_u32(__UINT32_TYPE__ value,
                                                     unsigned int count) {
  return (__UINT32_TYPE__)(value << count) | (value >> (32 - count));
}

[[unsequenced]]
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
[[unsequenced]]
static inline __UINT8_TYPE__ __stdc_rotate_right_u8(__UINT8_TYPE__ value,
                                                    unsigned int count) {
  return (value >> count) | (__UINT8_TYPE__)(value << (8 - count));
}

[[unsequenced]]
static inline __UINT16_TYPE__ __stdc_rotate_right_u16(__UINT16_TYPE__ value,
                                                      unsigned int count) {
  return (value >> count) | (__UINT16_TYPE__)(value << (16 - count));
}

[[unsequenced]]
static inline __UINT32_TYPE__ __stdc_rotate_right_u32(__UINT32_TYPE__ value,
                                                      unsigned int count) {
  return (value >> count) | (__UINT32_TYPE__)(value << (32 - count));
}

[[unsequenced]]
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

__attribute__((nonnull(1))) void bzero(void *dst, __SIZE_TYPE__ len);

__attribute__((nonnull(1))) void explicit_bzero(void *dst, __SIZE_TYPE__ len);

__attribute__((nonnull(1, 2))) void *
memcpy(void *restrict dst, const void *restrict src, __SIZE_TYPE__ len);

__attribute__((nonnull(1))) void *memset(void *restrict dst, int byte,
                                         __SIZE_TYPE__ len);

[[reproducible]]
__attribute__((nonnull(1, 2), pure)) int memcmp(const void *s1, const void *s2,
                                                __SIZE_TYPE__ n);

[[reproducible]]
__attribute__((nonnull(1), pure)) __SIZE_TYPE__ strlen(const char *s);

#endif // UKO_OS_KERNEL__COMPILER_H
