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

/**
 * String functions. These don't _look_ builtin (hey, we're defining them here
 * ourselves!), but the compiler's optimizer treats them as builtins, and can do
 * certain optimizations that it wouldn't otherwise be able to do.
 */

[[reproducible]]
int memcmp(const void *s1, const void *s2, __SIZE_TYPE__ n)
    __attribute__((nonnull(1, 2), pure));

[[reproducible]]
__SIZE_TYPE__ strlen(const char *s) __attribute__((nonnull(1), pure));

#endif // UKO_OS_KERNEL__COMPILER_H
