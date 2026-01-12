/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <arch/riscv64/insns.h>
#include <random.h>

void arch_entropy_pool_seed_early(void) {
  // TODO: Detect and use Zkr.

  // We can get some entropy from the boot time, but we're not sure how much, so
  // it's not worth crediting.
  u64 time_based_entropy[] = {csrr(RISCV64_CSR_CYCLE), csrr(RISCV64_CSR_TIME)};
  entropy_pool_mix((const u8 *)time_based_entropy, sizeof(time_based_entropy));
}
