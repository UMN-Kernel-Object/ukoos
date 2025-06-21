#include <types.h>

__attribute__((nonnull(1), pure)) usize strlen(const char *s) {
  usize n = 0;
  while (*s++)
    n++;
  return n;
}
