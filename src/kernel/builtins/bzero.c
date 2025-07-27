#include <types.h>

[[gnu::access(write_only, 1), gnu::nonnull(1)]]
void bzero(void *dst, usize len) {
  u8 *p = dst;
  while (len--)
    *p++ = 0;
}
