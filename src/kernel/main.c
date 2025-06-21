#include <panic.h>
#include <print.h>

void foo(void) { TODO(); }

[[noreturn]]
void main(void) {
  print("Starting to boot ukoOS...\n");
  for (;;)
    foo();
}
