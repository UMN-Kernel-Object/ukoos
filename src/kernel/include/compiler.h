#ifndef UKO_OS_KERNEL__COMPILER_H
#define UKO_OS_KERNEL__COMPILER_H 1

/**
 * Macros that expose compiler builtins.
 */

#define alignof _Alignof
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
#define stdc_rotate_left __builtin_stdc_rotate_left
#define stdc_rotate_right __builtin_stdc_rotate_right

/**
 * String functions. These don't _look_ builtin (hey, we're defining them here
 * ourselves!), but the compiler's optimizer treats them as builtins, and can do
 * certain optimizations that it wouldn't otherwise be able to do.
 */

__attribute__((nonnull(1))) void bzero(void *dst, __SIZE_TYPE__ len);

__attribute__((nonnull(1, 2))) void *
memcpy(void *restrict dst, const void *restrict src, __SIZE_TYPE__ len);

[[reproducible]]
__attribute__((nonnull(1, 2), pure)) int memcmp(const void *s1, const void *s2,
                                                __SIZE_TYPE__ n);

[[reproducible]]
__attribute__((nonnull(1), pure)) __SIZE_TYPE__ strlen(const char *s);

#endif // UKO_OS_KERNEL__COMPILER_H
