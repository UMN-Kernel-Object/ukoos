#include <types.h>

__attribute__((nonnull(1))) void *memset(void *restrict dst, int byte,
                                         usize len) {
  u8 *dst_ = dst;
  u8 byte_ = (u8)byte;
  while (len--)
    *dst_++ = byte_;
  return dst;
}
