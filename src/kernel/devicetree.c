#include <devicetree.h>
#include <physical.h>
#include <print.h>

struct fdt_header {
  u32 magic;
  u32 totalsize;
  u32 off_dt_struct;
  u32 off_dt_strings;
  u32 off_mem_rsvmap;
  u32 version;
  u32 last_comp_version;
  u32 boot_cpuid_phys;
  u32 size_dt_strings;
  u32 size_dt_struct;
};

struct fdt_reserve_entry {
  u64 address;
  u64 size;
};

enum fdt_struct_token_type : u32 {
  FDT_BEGIN_NODE = 1,
  FDT_END_NODE = 2,
  FDT_PROP = 3,
  FDT_NOP = 4,
  FDT_END = 9,
};

static paddr devicetree_start = {0};
static struct fdt_header devicetree_header = {0};

static struct fdt_header physical_read_fdt_header(paddr paddr) {
  return (struct fdt_header){
      .magic = physical_read_u32be(paddr_offset(paddr, 0)),
      .totalsize = physical_read_u32be(paddr_offset(paddr, 4)),
      .off_dt_struct = physical_read_u32be(paddr_offset(paddr, 8)),
      .off_dt_strings = physical_read_u32be(paddr_offset(paddr, 12)),
      .off_mem_rsvmap = physical_read_u32be(paddr_offset(paddr, 16)),
      .version = physical_read_u32be(paddr_offset(paddr, 20)),
      .last_comp_version = physical_read_u32be(paddr_offset(paddr, 24)),
      .boot_cpuid_phys = physical_read_u32be(paddr_offset(paddr, 28)),
      .size_dt_strings = physical_read_u32be(paddr_offset(paddr, 32)),
      .size_dt_struct = physical_read_u32be(paddr_offset(paddr, 36)),
  };
}

void devicetree_init(paddr start) {
  assert(!paddr_to_bits(devicetree_start), "DeviceTree already initialized");

  struct fdt_header header = physical_read_fdt_header(start);
  assert(header.magic == 0xd00dfeed,
         "DeviceTree magic value did not match (got {u32:#010x}, expected "
         "0xd00dfeed)",
         header.magic);
  assert(header.last_comp_version <= 17,
         "DeviceTree was not compatible with our parser");

  devicetree_start = start;
  devicetree_header = header;
}

/**
 * A function that gets called for each discovered memory range. It finds the
 * subranges that are actually available for use by allocation, and gives them
 * to the allocator.
 */
static void devicetree_mm_init_on_mem_range(paddr start, usize len) {
  print("Found memory range: {paddr}-{paddr}", start, paddr_offset(start, len));
}

void devicetree_mm_init(void) {
  assert(paddr_to_bits(devicetree_start), "DeviceTree not initialized");

  // Parse the structure block, loooking for the memory node.
  u32 address_cells = 0, size_cells = 0;
  paddr reg_start = {0};
  usize reg_len = 0, depth = 0;

  bool has_device_type_memory = false, has_reg = false,
       processed_memory_node = false;
  paddr struct_start =
      paddr_offset(devicetree_start, devicetree_header.off_dt_struct);
  paddr here = struct_start;
  while (paddr_to_bits(here) !=
         paddr_to_bits(struct_start) + devicetree_header.size_dt_struct) {

    // Parse token by token.
    enum fdt_struct_token_type token_type = physical_read_u32be(here);
    here = paddr_offset(here, 4);
    switch (token_type) {
    case FDT_BEGIN_NODE: {
      // Skip the name.
      while (physical_read_u8(here))
        here = paddr_offset(here, 1);
      here = paddr_align_up(paddr_offset(here, 1), 2);

      // Track the depth and reset information about props.
      has_reg = false;
      has_device_type_memory = false;
      processed_memory_node = false;
      depth++;
    } break;

      // Track the depth and reset information about props.
    case FDT_END_NODE:
      assert(depth > 0);
      has_reg = false;
      has_device_type_memory = false;
      processed_memory_node = false;
      depth--;
      break;

    case FDT_PROP: {
      // Read the fixed fields.
      u32 value_len = physical_read_u32be(here);
      here = paddr_offset(here, 4);
      u32 name_off = physical_read_u32be(here);
      here = paddr_offset(here, 4);

      // Find the bounds of the name.
      paddr name_start = paddr_offset(
          devicetree_start, devicetree_header.off_dt_strings + name_off);
      paddr name_end = name_start;
      while (physical_read_u8(name_end))
        name_end = paddr_offset(name_end, 1);
      usize name_len = paddr_to_bits(name_end) - paddr_to_bits(name_start);

      // Skip the value.
      paddr value_start = here;
      here = paddr_align_up(paddr_offset(here, value_len), 2);

      // Read the property name in, if it's short enough.
      char name[16] = {0};
      if (name_len < 16) {
        copy_from_physical(name, name_start, name_len);

        // Try to recognize the property from its name.
        switch (name_len) {
        case 3:
          if (!memcmp(name, "reg", 3)) {
            reg_start = value_start;
            reg_len = value_len;
            has_reg = true;
          }
          break;
        case 11:
          if (!memcmp(name, "#size-cells", 11)) {
            assert(value_len == 4);
            if (depth == 1) {
              size_cells = physical_read_u32be(value_start);
              assert(size_cells == 1 || size_cells == 2);
            }
          } else if (!memcmp(name, "device_type", 11)) {
            if (value_len == 7) {
              char device_type[7];
              copy_from_physical(device_type, value_start, 7);
              if (!memcmp(device_type, "memory", 7)) {
                has_device_type_memory = true;
              }
            }
          }
          break;
        case 14:
          if (!memcmp(name, "#address-cells", 14)) {
            assert(value_len == 4);
            if (depth == 1) {
              address_cells = physical_read_u32be(value_start);
              assert(address_cells == 1 || address_cells == 2);
            }
          }
          break;
        }

        // Check if we found a memory node.
        if (has_reg && has_device_type_memory && !processed_memory_node) {
          usize entry_size = (address_cells + size_cells) * 4;
          assert((reg_len % entry_size) == 0);

          // Iterate over individual entries of the reg property.
          u64 addr = 0, size = 0;
          paddr reg = reg_start;
          paddr reg_end = paddr_offset(reg_start, reg_len);
          while (paddr_to_bits(reg) != paddr_to_bits(reg_end)) {
            for (u32 i = 0; i < address_cells; i++) {
              addr = (addr << 32) | (u64)physical_read_u32be(reg);
              reg = paddr_offset(reg, 4);
            }
            for (u32 i = 0; i < size_cells; i++) {
              size = (size << 32) | (u64)physical_read_u32be(reg);
              reg = paddr_offset(reg, 4);
            }

            devicetree_mm_init_on_mem_range(paddr_of_bits(addr), size);
          }

          // Set a flag so we don't re-process the node for every additional
          // prop we find.
          processed_memory_node = true;
        }
      }
    } break;

      // Just skip a no-op.
    case FDT_NOP:
      break;

      // When we get to the end, check that we're in the right place to stop.
    case FDT_END:
      assert(paddr_to_bits(here) ==
             paddr_to_bits(struct_start) + devicetree_header.size_dt_struct);
      break;

    default:
      panic("unknown token type in structure block: {u32}", token_type);
    }
  }
}
