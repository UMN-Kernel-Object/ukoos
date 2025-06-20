#include <arch/riscv64/insns.h>
#include <arch/riscv64/sbi.h>

noreturn void _panic_halt(void) {
  sbi_call(0x53525354, 0, 0, 1);
  for (;;) {
    wfi();
  }
}
