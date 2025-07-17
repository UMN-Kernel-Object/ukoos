#ifndef UKO_OS_KERNEL__MACRO_UTILS_H
#define UKO_OS_KERNEL__MACRO_UTILS_H 1

#define __HACKY_INTEGRAL_TYPEID(EXPR)                                          \
  _Generic((EXPR),                                                             \
      i8: 1,                                                                   \
      u8: 0,                                                                   \
      i16: 3,                                                                  \
      u16: 2,                                                                  \
      i32: 5,                                                                  \
      u32: 4,                                                                  \
      i64: 9,                                                                  \
      u64: 8,                                                                  \
      isize: 7,                                                                \
      usize: 6)

#define __PASTE(L, R) L##R

#endif // UKO_OS_KERNEL__MACRO_UTILS_H
