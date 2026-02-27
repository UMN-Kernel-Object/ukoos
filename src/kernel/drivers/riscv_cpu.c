/*
 * SPDX-FileCopyrightText: ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <arch/riscv64/devices/hart.h>
#include <devices/hart.h>
#include <devicetree.h>
#include <endian.h>
#include <mm/paging.h>
#include <physical.h>

struct riscv_cpu {
  struct device device;
  struct hart hart;

  atomic struct riscv_hart_group_key hart_group_key;
};

static bool extensions_list_has_extension(const u8 *value, usize value_len,
                                          const char *extension_name) {
  usize extension_name_len = strlen(extension_name);
  while (value_len) {
    usize ext_len = strnlen((const char *)value, value_len);
    if (ext_len == extension_name_len &&
        memcmp(extension_name, value, ext_len) == 0) {
      return true;
    }
    value += ext_len + 1;
    value_len -= ext_len + 1;
  }
  return false;
}

static u32 get_cboz_block_size_from_extensions(
    struct devicetree_node *node, struct devicetree_prop *prop_isa_extensions) {
  u32 cboz_block_size = 0;
  if (extensions_list_has_extension(prop_isa_extensions->value,
                                    prop_isa_extensions->value_len, "zicboz")) {
    u32 block_size;
    struct devicetree_prop *prop_cboz_block_size =
        devicetree_prop(node, "riscv,cboz-block-size");
    if (prop_cboz_block_size) {
      if (prop_cboz_block_size->value_len == sizeof(block_size)) {
        memcpy(&block_size, prop_cboz_block_size->value, sizeof(block_size));
        block_size = big_to_native(block_size);

        cboz_block_size = block_size;
      } else {
        print("Devicetree device {cstr} did not have the riscv,cboz-block-size "
              "property",
              node->name);
      }
    } else {
      print("Devicetree device {cstr} had the wrong size for the "
            "riscv,cboz-block-size property",
            node->name);
    }
  }
  return cboz_block_size;
}

static enum riscv_flen
get_flen_from_extensions(struct devicetree_prop *prop_isa_extensions) {
  if (extensions_list_has_extension(prop_isa_extensions->value,
                                    prop_isa_extensions->value_len, "q")) {
    return FLEN_128;
  } else if (extensions_list_has_extension(prop_isa_extensions->value,
                                           prop_isa_extensions->value_len,
                                           "d")) {
    return FLEN_64;
  } else if (extensions_list_has_extension(prop_isa_extensions->value,
                                           prop_isa_extensions->value_len,
                                           "f")) {
    return FLEN_32;
  } else {
    return FLEN_0;
  }
}

static enum riscv_vlen
get_vlen_from_extensions(struct devicetree_node *node,
                         struct devicetree_prop *prop_isa_extensions) {
  enum riscv_vlen vlen;

  if (extensions_list_has_extension(prop_isa_extensions->value,
                                    prop_isa_extensions->value_len, "v")) {
    vlen = VLEN_UNKNOWN;

    u32 vlenb;
    struct devicetree_prop *prop_thead_vlenb =
        devicetree_prop(node, "thead,vlenb");
    if (prop_thead_vlenb) {
      if (prop_thead_vlenb->value_len == sizeof(vlenb)) {
        memcpy(&vlenb, prop_thead_vlenb->value, sizeof(vlenb));
        vlenb = big_to_native(vlenb);

        switch (vlenb) {
        case 128:
          vlen = VLEN_128;
          break;
        case 256:
          vlen = VLEN_256;
          break;
        case 512:
          vlen = VLEN_512;
          break;
        case 1024:
          vlen = VLEN_1024;
          break;
        case 2048:
          vlen = VLEN_2048;
          break;
        case 4096:
          vlen = VLEN_4096;
          break;
        case 8192:
          vlen = VLEN_8192;
          break;
        case 16384:
          vlen = VLEN_16384;
          break;
        case 32768:
          vlen = VLEN_32768;
          break;
        case 65536:
          vlen = VLEN_65536;
          break;
        default:
          print("Devicetree device {cstr} had a spec-disallowed value for the "
                "thead,vlenb property ({u32})",
                node->name, vlenb);
        }
      } else {
        print("Devicetree device {cstr} had the wrong size for the thead,vlenb "
              "property",
              node->name);
      }
    } else {
      print("Devicetree device {cstr} did not have the thead,vlenb property",
            node->name);
      vlen = VLEN_1024;
    }
  } else {
    vlen = VLEN_0;
  }

  return vlen;
}

static struct device *riscv_cpu_enumerate_dt(struct devicetree_node *node) {
  struct riscv_cpu *device = nullptr;
  struct devicetree_prop *prop_reg, *prop_isa_base, *prop_isa_extensions;
  u32 id;

  // Get the hart number.
  prop_reg = devicetree_prop(node, "reg");
  if (!prop_reg) {
    print("Devicetree device {cstr} was missing the reg property", node->name);
    goto fail;
  }
  if (prop_reg->value_len != sizeof(id)) {
    print("Devicetree device {cstr} had the wrong size for the reg property",
          node->name);
    goto fail;
  }
  memcpy(&id, prop_reg->value, sizeof(id));
  id = big_to_native(id);

  // Check for the ISA the hart supports.
  prop_isa_base = devicetree_prop(node, "riscv,isa-base");
  prop_isa_extensions = devicetree_prop(node, "riscv,isa-extensions");
  if (!prop_isa_base || !prop_isa_extensions) {
    if (devicetree_prop(node, "riscv,isa")) {
      print("Devicetree device {cstr} has the riscv,isa property but not "
            "riscv,isa-base and riscv,isa-extensions; update the Devicetree",
            node->name);
    }
    goto fail;
  }

  if (prop_isa_base->value_len != 6 ||
      memcmp(prop_isa_base->value, "rv64i", 6) != 0) {
    print("Devicetree device {cstr} was not rv64i", node->name);
    goto fail;
  }

  // Allocate the device.
  device = alloc(sizeof(struct riscv_cpu));
  if (!device)
    goto fail;
  *device = (struct riscv_cpu){
      .device = DEVICE_INIT(device->device, "riscv_cpu@{u32}", id),
      .hart =
          {
              .list = LIST_INIT(device->hart.list),
              .device = &device->device,
              .id = id,
              .hart_group = nullptr,
          },
      .hart_group_key =
          {
              .cboz_block_size = get_cboz_block_size_from_extensions(
                  node, prop_isa_extensions),
              .flen = get_flen_from_extensions(prop_isa_extensions),
              .vlen = get_vlen_from_extensions(node, prop_isa_extensions),
          },
  };
  if (!device->device.name)
    goto fail;

  // Try to assign a hart group now, if we can.
  if (riscv_hart_group_key_is_fully_determined(device->hart_group_key)) {
    struct riscv_hart_group *hart_group =
        riscv_hart_group_get(device->hart_group_key);
    if (!hart_group)
      goto fail;
    device->hart.hart_group = &hart_group->hart_group;
  }

  // Add the device to the list of harts and return.
  list_push(&harts, &device->hart.list);
  return &device->device;

fail:
  if (device) {
    free(device->device.name);
  }
  free(device);
  return nullptr;
}

DEFINE_INIT(INIT_REGISTER_DRIVERS) {
  devicetree_register("riscv", riscv_cpu_enumerate_dt);
}
