#include <types.h>

__attribute__((nonnull(1, 2))) void *
memcpy(void *restrict dst, const void *restrict src, usize len) {
  u8 *dst_ = dst;
  const u8 *src_ = src;
  while (len--)
    *dst_++ = *src_++;
  return dst;
}
