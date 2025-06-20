#ifndef UKO_OS_KERNEL__ARCH_RISCV64_SBI_H
#define UKO_OS_KERNEL__ARCH_RISCV64_SBI_H 1

/**
 * Bindings to the RISC-V Supervisor Binary Interface specification. These are
 * low-level bindings, to be used by drivers for specific SBI functionality.
 */

#include <types.h>

/**
 * An error code returned by an SBI call.
 */
enum sbi_error : iaddr {
  SBI_SUCCESS = 0,                // Completed successfully
  SBI_ERR_FAILED = -1,            // Failed
  SBI_ERR_NOT_SUPPORTED = -2,     // Not supported
  SBI_ERR_INVALID_PARAM = -3,     // Invalid parameter(s)
  SBI_ERR_DENIED = -4,            // Denied or not allowed
  SBI_ERR_INVALID_ADDRESS = -5,   // Invalid address(s)
  SBI_ERR_ALREADY_AVAILABLE = -6, // Already available
  SBI_ERR_ALREADY_STARTED = -7,   // Already started
  SBI_ERR_ALREADY_STOPPED = -8,   // Already stopped
  SBI_ERR_NO_SHMEM = -9,          // Shared memory not available
};

/**
 * The return value of an SBI call.
 */
struct sbi_ret {
  enum sbi_error error;
  uaddr value;
};

static inline struct sbi_ret sbi_call_0(uaddr eid, uaddr fid) {
  register uaddr a0 __asm__("a0");
  register uaddr a1 __asm__("a1");
  register uaddr a6 __asm__("a6") = fid;
  register uaddr a7 __asm__("a7") = eid;

  __asm__ volatile("ecall\n"
                   : "=r"(a0), "=r"(a1)
                   : "r"(a6), "r"(a7)
                   : "memory");

  return (struct sbi_ret){.error = (enum sbi_error)a0, .value = a1};
}

static inline struct sbi_ret sbi_call_1(uaddr eid, uaddr fid, uaddr arg0) {
  register uaddr a0 __asm__("a0") = arg0;
  register uaddr a1 __asm__("a1");
  register uaddr a6 __asm__("a6") = fid;
  register uaddr a7 __asm__("a7") = eid;

  __asm__ volatile("ecall\n"
                   : "+r"(a0), "=r"(a1)
                   : "r"(a6), "r"(a7)
                   : "memory");

  return (struct sbi_ret){.error = (enum sbi_error)a0, .value = a1};
}

static inline struct sbi_ret sbi_call_2(uaddr eid, uaddr fid, uaddr arg0,
                                        uaddr arg1) {
  register uaddr a0 __asm__("a0") = arg0;
  register uaddr a1 __asm__("a1") = arg1;
  register uaddr a6 __asm__("a6") = fid;
  register uaddr a7 __asm__("a7") = eid;

  __asm__ volatile("ecall\n"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a6), "r"(a7)
                   : "memory");

  return (struct sbi_ret){.error = (enum sbi_error)a0, .value = a1};
}

static inline struct sbi_ret sbi_call_3(uaddr eid, uaddr fid, uaddr arg0,
                                        uaddr arg1, uaddr arg2) {
  register uaddr a0 __asm__("a0") = arg0;
  register uaddr a1 __asm__("a1") = arg1;
  register uaddr a2 __asm__("a2") = arg2;
  register uaddr a6 __asm__("a6") = fid;
  register uaddr a7 __asm__("a7") = eid;

  __asm__ volatile("ecall\n"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a2), "r"(a6), "r"(a7)
                   : "memory");

  return (struct sbi_ret){.error = (enum sbi_error)a0, .value = a1};
}

static inline struct sbi_ret sbi_call_4(uaddr eid, uaddr fid, uaddr arg0,
                                        uaddr arg1, uaddr arg2, uaddr arg3) {
  register uaddr a0 __asm__("a0") = arg0;
  register uaddr a1 __asm__("a1") = arg1;
  register uaddr a2 __asm__("a2") = arg2;
  register uaddr a3 __asm__("a3") = arg3;
  register uaddr a6 __asm__("a6") = fid;
  register uaddr a7 __asm__("a7") = eid;

  __asm__ volatile("ecall\n"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a2), "r"(a3), "r"(a6), "r"(a7)
                   : "memory");

  return (struct sbi_ret){.error = (enum sbi_error)a0, .value = a1};
}

static inline struct sbi_ret sbi_call_5(uaddr eid, uaddr fid, uaddr arg0,
                                        uaddr arg1, uaddr arg2, uaddr arg3,
                                        uaddr arg4) {
  register uaddr a0 __asm__("a0") = arg0;
  register uaddr a1 __asm__("a1") = arg1;
  register uaddr a2 __asm__("a2") = arg2;
  register uaddr a3 __asm__("a3") = arg3;
  register uaddr a4 __asm__("a4") = arg4;
  register uaddr a6 __asm__("a6") = fid;
  register uaddr a7 __asm__("a7") = eid;

  __asm__ volatile("ecall\n"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a2), "r"(a3), "r"(a4), "r"(a6), "r"(a7)
                   : "memory");

  return (struct sbi_ret){.error = (enum sbi_error)a0, .value = a1};
}

static inline struct sbi_ret sbi_call_6(uaddr eid, uaddr fid, uaddr arg0,
                                        uaddr arg1, uaddr arg2, uaddr arg3,
                                        uaddr arg4, uaddr arg5) {
  register uaddr a0 __asm__("a0") = arg0;
  register uaddr a1 __asm__("a1") = arg1;
  register uaddr a2 __asm__("a2") = arg2;
  register uaddr a3 __asm__("a3") = arg3;
  register uaddr a4 __asm__("a4") = arg4;
  register uaddr a5 __asm__("a5") = arg5;
  register uaddr a6 __asm__("a6") = fid;
  register uaddr a7 __asm__("a7") = eid;

  __asm__ volatile("ecall\n"
                   : "+r"(a0), "+r"(a1)
                   : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                   : "memory");

  return (struct sbi_ret){.error = (enum sbi_error)a0, .value = a1};
}

#define _sbi_seventh(_0, _1, _2, _3, _4, _5, N, ...) N
#define _sbi_count_args(...)                                                   \
  _sbi_seventh(__VA_ARGS__ __VA_OPT__(, ) 6, 5, 4, 3, 2, 1, 0)
#define __sbi_call(EID, FID, N, ...)                                           \
  sbi_call_##N(EID, FID __VA_OPT__(, ) __VA_ARGS__)
#define _sbi_call(EID, FID, N, ...) __sbi_call(EID, FID, N, __VA_ARGS__)

/**
 * Performs a call to the SBI firmware.
 */
#define sbi_call(EID, FID, ...)                                                \
  _sbi_call(EID, FID, _sbi_count_args(__VA_ARGS__) __VA_OPT__(, ) __VA_ARGS__)

#endif // UKO_OS_KERNEL__ARCH_RISCV64_SBI_H
