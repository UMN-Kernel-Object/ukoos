#include <arch/riscv64/insns.h>
#include <panic.h>

enum trap_cause {
  // interrupts
  SUPERVISOR_SOFTWARE_INTERRUPT = 1 << 63 | 1,
  SUPERVISOR_TIMER_INTERRUPT = 1 << 63 | 5,
  SUPERVISOR_EXTERNAL_INTERRUPT = 1 << 63 | 9,
  COUNTERFLOW_INTERRUPT = 1 << 63 | 13,

  // exceptions
  // TODO: all the exceptions
  // the following is a placeholder
  EXCEPTION = 0 << 63,
};

void trap_handler() {
  u64 scause = csr(RISCV64_CSR_SCAUSE);
  switch (scause) {
  case SUPERVISOR_SOFTWARE_INTERRUPT:
    TODO();
  case SUPERVISOR_TIMER_INTERRUPT:
    TODO();
  case SUPERVISOR_EXTERNAL_INTERRUPT:
    TODO();
  case COUNTERFLOW_INTERRUPT:
    TODO();
  }
}
