#include <physical.h>

static void put_byte(u8 byte) {
  physical_write_u8(paddr_of_bits(0x10000000), byte);
}

static void puts(const char *s) {
  char ch;
  while ((ch = *s++))
    put_byte(ch);
}

void main(void) {
  puts("Hello, world!\n");
  panic();
}
