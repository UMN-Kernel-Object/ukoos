#include <physical.h>

constexpr paddr UART0 = {.bits = 0x10000000};

static void put_byte(u8 byte) { physical_write_u8(UART0, byte); }

static void puts(const char *s) {
  char ch;
  while ((ch = *s++))
    put_byte(ch);
}

void main(void) {
  for (;;)
    puts("Hello, world!\n");
}
