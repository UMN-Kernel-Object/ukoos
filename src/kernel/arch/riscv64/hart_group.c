/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <arch/riscv64/drivers/riscv_cpu.h>
#include <mm/alloc.h>

// TODO: This should become a hashtable, but this isn't _so_ horrible right now,
// since there won't be very many hart groups or harts.

struct riscv_hart_group_list {
  struct riscv_hart_group_list *next;
  struct riscv_hart_group_key key;
  struct riscv_hart_group group;
};

static struct riscv_hart_group_list *hart_groups = nullptr;

static bool riscv_hart_group_key_eq(struct riscv_hart_group_key key1,
                                    struct riscv_hart_group_key key2) {
  return key1.cboz_block_size == key2.cboz_block_size &&
         key1.flen == key2.flen && key1.vlen == key2.vlen;
}

static void riscv_hart_group_make(struct riscv_hart_group *group,
                                  struct riscv_hart_group_key key) {
  usize len = 0;

  // X
  group->x_off = len;
  group->x_len = 32 * sizeof(u64); // We don't support rv32, rv64e, or rv64y
  len += group->x_len;

  // F
  if (key.flen != FLEN_0) {
    usize sizeof_f;
    switch (key.flen) {
    case FLEN_0:
      sizeof_f = 0;
      break;
    case FLEN_32:
      sizeof_f = 4;
      break;
    case FLEN_64:
      sizeof_f = 8;
      break;
    case FLEN_128:
      sizeof_f = 16;
      break;
    }
    len = (len + sizeof_f - 1) & ~(sizeof_f - 1);
    group->f_off = len;
    group->f_len = 32 * sizeof_f;
    len += group->f_len;
  } else {
    group->f_off = 0;
    group->f_len = 0;
  }

  // V
  if (key.vlen != VLEN_0) {
    usize sizeof_v;
    switch (key.vlen) {
    case VLEN_UNKNOWN:
    default:
      panic("hart group key was not fully determined");
    case VLEN_0:
      sizeof_v = 0;
      break;
    case VLEN_128:
      sizeof_v = 16;
      break;
    case VLEN_256:
      sizeof_v = 32;
      break;
    case VLEN_512:
      sizeof_v = 64;
      break;
    case VLEN_1024:
      sizeof_v = 128;
      break;
    case VLEN_2048:
      sizeof_v = 256;
      break;
    case VLEN_4096:
      sizeof_v = 512;
      break;
    case VLEN_8192:
      sizeof_v = 1024;
      break;
    case VLEN_16384:
      sizeof_v = 2048;
      break;
    case VLEN_32768:
      sizeof_v = 4096;
      break;
    case VLEN_65536:
      sizeof_v = 8192;
      break;
    }
    len = (len + 7) & ~(usize)7;
    group->v_off = len;
    group->v_len = 32 * sizeof_v;
    len += group->v_len;
  } else {
    group->v_off = 0;
    group->v_len = 0;
  }

  group->hart_group.sizeof_register_save_area = len;
}

struct riscv_hart_group *riscv_hart_group_get(struct riscv_hart_group_key key) {
  struct riscv_hart_group_list *link;

  for (link = hart_groups; link; link = link->next)
    if (riscv_hart_group_key_eq(key, link->key))
      return &link->group;

  link = alloc(sizeof(struct riscv_hart_group_list));
  if (!link)
    return nullptr;
  link->next = hart_groups;
  link->key = key;
  riscv_hart_group_make(&link->group, key);
  hart_groups = link;
  return &link->group;
}
