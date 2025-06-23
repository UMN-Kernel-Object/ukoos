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

struct struct_callbacks {
  void (*on_begin_node)(paddr name_start, usize name_len, void *ctx);
  void (*on_end_node)(void *ctx);
  void (*on_prop)(paddr name_start, usize name_len, paddr value_start,
                  usize value_len, void *ctx);
  // TODO
  void *ctx;
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

static void parse_struct_block(struct struct_callbacks *callbacks) {
  paddr struct_start =
      paddr_offset(devicetree_start, devicetree_header.off_dt_struct);
  paddr here = struct_start;
  while (paddr_to_bits(here) !=
         paddr_to_bits(struct_start) + devicetree_header.size_dt_struct) {
    enum fdt_struct_token_type token_type = physical_read_u32be(here);
    here = paddr_offset(here, 4);
    switch (token_type) {
    case FDT_BEGIN_NODE: {
      paddr name_start = here;
      while (physical_read_u8(here))
        here = paddr_offset(here, 1);
      paddr name_end = here;
      here = paddr_align_up(paddr_offset(here, 1), 2);
      callbacks->on_begin_node(
          name_start, paddr_to_bits(name_end) - paddr_to_bits(name_start),
          callbacks->ctx);
    } break;
    case FDT_END_NODE:
      callbacks->on_end_node(callbacks->ctx);
      break;
    case FDT_PROP: {
      u32 value_len = physical_read_u32be(here);
      here = paddr_offset(here, 4);
      u32 name_off = physical_read_u32be(here);
      here = paddr_offset(here, 4);

      paddr name_start = paddr_offset(
          devicetree_start, devicetree_header.off_dt_strings + name_off);
      paddr name_end = name_start;
      while (physical_read_u8(name_end))
        name_end = paddr_offset(name_end, 1);

      paddr value_start = here;
      here = paddr_align_up(paddr_offset(here, value_len), 2);

      callbacks->on_prop(name_start,
                         paddr_to_bits(name_end) - paddr_to_bits(name_start),
                         value_start, value_len, callbacks->ctx);
    } break;
    case FDT_NOP:
      break;
    case FDT_END:
      assert(paddr_to_bits(here) ==
             paddr_to_bits(struct_start) + devicetree_header.size_dt_struct);
      break;
    default:
      panic("unknown token type in structure block: {u32}", token_type);
    }
  }
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

struct mm_init_state {
  u32 address_cells, size_cells;
  paddr reg_start;
  usize reg_len;
  usize depth;

  bool has_reg;
  bool in_memory;
};

static void devicetree_mm_init_on_mem_range(paddr start, usize len) {
  print("Found memory range: {paddr}-{paddr}", start, paddr_offset(start, len));
}

static void devicetree_mm_init_on_begin_node(paddr name_start, usize name_len,
                                             void *ctx) {
  struct mm_init_state *state = ctx;
  state->has_reg = false;
  state->in_memory = false;
  state->depth++;
}

static void devicetree_mm_init_on_end_node(void *ctx) {
  struct mm_init_state *state = ctx;
  assert(state->depth > 0);
  state->depth--;
}

static void devicetree_mm_init_on_prop(paddr name_start, usize name_len,
                                       paddr value_start, usize value_len,
                                       void *ctx) {
  struct mm_init_state *state = ctx;
  char name[16] = {0};
  if (name_len > 15)
    return;
  copy_from_physical(name, name_start, name_len);

  // Try to recognize the property from its name.
  switch (name_len) {
  case 3:
    if (!memcmp(name, "reg", 3)) {
      state->reg_start = value_start;
      state->reg_len = value_len;
      state->has_reg = true;
    }
    break;
  case 11:
    if (!memcmp(name, "#size-cells", 11)) {
      assert(value_len == 4);
      if (state->depth == 1) {
        state->size_cells = physical_read_u32be(value_start);
        assert(state->size_cells == 1 || state->size_cells == 2);
      }
    } else if (!memcmp(name, "device_type", 11)) {
      if (value_len == 7) {
        char device_type[7];
        copy_from_physical(device_type, value_start, 7);
        if (!memcmp(device_type, "memory", 7)) {
          state->in_memory = true;
        }
      }
    }
    break;
  case 14:
    if (!memcmp(name, "#address-cells", 14)) {
      assert(value_len == 4);
      if (state->depth == 1) {
        state->address_cells = physical_read_u32be(value_start);
        assert(state->address_cells == 1 || state->address_cells == 2);
      }
    }
    break;
  }

  // Check if we found a memory node.
  if (state->has_reg && state->in_memory) {
    usize entry_size = (state->address_cells + state->size_cells) * 4;
    assert((state->reg_len % entry_size) == 0);

    u64 addr = 0, size = 0;
    paddr reg = state->reg_start;
    paddr reg_end = paddr_offset(state->reg_start, state->reg_len);
    while (paddr_to_bits(reg) != paddr_to_bits(reg_end)) {
      for (u32 i = 0; i < state->address_cells; i++) {
        addr = (addr << 32) | (u64)physical_read_u32be(reg);
        reg = paddr_offset(reg, 4);
      }
      for (u32 i = 0; i < state->size_cells; i++) {
        size = (size << 32) | (u64)physical_read_u32be(reg);
        reg = paddr_offset(reg, 4);
      }

      devicetree_mm_init_on_mem_range(paddr_of_bits(addr), size);
    }
  }
}

void devicetree_mm_init(void) {
  assert(paddr_to_bits(devicetree_start), "DeviceTree not initialized");

  // Parse the structure block, loooking for the memory node.
  struct mm_init_state state = {};
  struct struct_callbacks callbacks = {
      .on_begin_node = devicetree_mm_init_on_begin_node,
      .on_end_node = devicetree_mm_init_on_end_node,
      .on_prop = devicetree_mm_init_on_prop,
      .ctx = &state,
  };
  parse_struct_block(&callbacks);
}
