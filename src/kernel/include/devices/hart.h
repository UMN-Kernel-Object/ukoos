/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UKO_OS_KERNEL__DEVICES_HART_H
#define UKO_OS_KERNEL__DEVICES_HART_H 1

#include <list.h>

// Avoids a circular include:
//   devices/hart.h -> device.h       for struct device
//   device.h -> print.h              for format()
//   print.h -> mm/alloc.h            for free()
//   mm/alloc.h -> hart_locals.h      for get_hart_locals().heap
//   hart_locals.h -> devices/hart.h  for struct hart
struct device;

struct hart_ops;

/**
 * A hart device.
 *
 * Currently, these devices are for "normal" SMP systems, where each hart runs
 * the same ukoOS kernel, makes its own scheduling decisions, etc. When we
 * support "peripheral-like harts," it might make sense to give them their own
 * device class rather than reusing this one.
 */
struct hart {
  /**
   * The list head used to link the hart into the `harts` list.
   */
  struct list_head list;

  /**
   * A pointer to the device.
   */
  struct device *device;

  /**
   * The hart's vtable.
   */
  const struct hart_ops *hart_ops;

  /**
   * The hart ID for this hart. Not guaranteed to be small or densely packed.
   */
  u64 id;

  /**
   * A pointer to the hart_group. This can be nullptr -- if it is, that means
   * that the hart_group is not yet detectable (e.g. if it can only be
   * determined after the hart boots and it has not yet booted).
   */
  struct hart_group *hart_group;
};

/**
 * The operations for a hart.
 */
struct hart_ops {
  /**
   * Fills in the hart_group field by detecting information about the current
   * hart.
   *
   * Requires:
   *
   * - `this->hart_group == nullptr`
   * - `this == get_hart_locals()->hart`
   *
   * Ensures:
   *
   * - `this->hart_group != nullptr`
   */
  void (*detect_hart_group)(struct hart *this);
};

/**
 * The list of all hart devices.
 */
extern struct list_head harts;

/**
 * A hart group contains shared data for harts that should be able to run the
 * same tasks as each other.
 *
 * In an SMP system, there will be only one hart group, since any task can be
 * run on any hart.
 *
 * An example of a more complicated system is the SpacemiT K3. This SoC has 8
 * X100 cores, which have 256-bit vector regiters, and 8 A100 cores, which have
 * 1024-bit vector registers. This would result in two hart groups, one for the
 * X100 cores and one for the A100 cores.
 *
 * TODO: We just leak these for now. If we support CPU hotplug, we shouldn't do
 * that, but that's not a near-term task.
 */
struct hart_group {
  /**
   * The total size of the register save area in `struct task` for tasks that
   * run on this hart group, including any padding needed.
   */
  usize sizeof_register_save_area;
};

#endif // UKO_OS_KERNEL__DEVICES_HART_H
