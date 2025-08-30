#include <types.h>

[[gnu::nonnull(1), gnu::pure]] usize strlen(const char *s) {
  usize n = 0;
  while (*s++)
    n++;
  return n;
}
