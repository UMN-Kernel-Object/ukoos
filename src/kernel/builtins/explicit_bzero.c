#include <types.h>

__attribute__((nonnull(1))) void explicit_bzero(void *dst, usize len) {
  bzero(dst, len);
  __asm__ volatile("" ::"r"(dst) : "memory");
}
