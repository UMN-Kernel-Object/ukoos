#include <types.h>

__attribute__((nonnull(1))) void bzero(void *dst, usize len) {
  u8 *p = dst;
  while (len--)
    *p++ = 0;
}
