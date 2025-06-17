typedef unsigned char u8;

static void put_byte(u8 byte) {
  volatile u8 *UART0 = (volatile u8 *)0x10000000;
  *UART0 = byte;
}

static void puts(const char *s) {
  char ch;
  while ((ch = *s++))
    put_byte(ch);
}

void main(void) {
  for (;;)
    puts("Hello, world!\n");
}
