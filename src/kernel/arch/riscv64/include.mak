# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

kernel-cflags += \
	-DARCH_RISCV64 \
	-fno-omit-frame-pointer \
	-mabi=lp64 \
	-mcmodel=medany
kernel-objs-asm += arch/riscv64/start
kernel-objs-c += arch/riscv64/backtrace
kernel-objs-c += arch/riscv64/hart_group
kernel-objs-c += arch/riscv64/hart_locals
kernel-objs-c += arch/riscv64/paging
kernel-objs-c += arch/riscv64/panic
kernel-objs-c += arch/riscv64/physical
kernel-objs-c += arch/riscv64/random
kernel-objs-c += arch/riscv64/sbi

$(eval $(call compute_component_variables,kernel))

all:: src/kernel/kernel.elf src/kernel/kernel.sym
install:: src/kernel/kernel.elf src/kernel/kernel.sym
	install -DT src/kernel/kernel.elf $(DESTDIR)$(prefix)/sys/kernel.elf
	install -DT src/kernel/kernel.sym $(DESTDIR)$(prefix)/sys/kernel.sym

$(call defcleanable, \
	src/kernel/arch/riscv64/bootstub.d \
	src/kernel/arch/riscv64/bootstub.o \
	src/kernel/arch/riscv64/bootstub_generated.ld \
	src/kernel/arch/riscv64/bootstub_generated.o \
	src/kernel/arch/riscv64/bootstub_generated.S \
	src/kernel/arch/riscv64/kernel-unstripped.elf \
	src/kernel/arch/riscv64/kernel.elf \
	src/kernel/kernel.elf \
	src/kernel/kernel.sym)

# Targets for booting and debugging the kernel.
gdb: src/kernel/kernel.sym
	$(GDB) src/kernel/kernel.sym \
		-ex "set substitute-path / $(srcdir)/" \
		-ex "layout src" \
		-ex "focus cmd" \
		-ex "target remote :1234" \
		-ex "break main" \
		-ex "break _panic_halt" \
		-ex "break unrecoverable_exception"
gdb_bootstub: src/kernel/kernel.sym
	$(GDB) src/kernel/kernel.sym \
		-ex "set substitute-path / $(srcdir)/" \
		-ex "layout asm" \
		-ex "layout regs" \
		-ex "focus cmd" \
		-ex "target remote :1234" \
		-ex "break *0x80080000" \
		-ex "break main"
qemu: src/kernel/kernel.elf
	qemu-system-riscv64 \
		-nographic \
		-kernel src/kernel/kernel.elf \
		$(target-qemuflags) \
		$(QEMUFLAGS)
qemu-debug: src/kernel/kernel.elf
	qemu-system-riscv64 \
		-nographic \
		-kernel src/kernel/kernel.elf \
		-s -S \
		$(target-qemuflags) \
		$(QEMUFLAGS)
.PHONY: gdb gdb_bootstub qemu qemu-debug

# A target for dumping the kernel.
objdump: src/kernel/arch/riscv64/kernel-unstripped.elf
	$(OBJDUMP) -d src/kernel/arch/riscv64/kernel-unstripped.elf | less
.PHONY: objdump

# Link the kernel. This kernel will have debug symbols, and not actually be
# bootable -- it depends on being loaded by and having the boot environment set
# up by the bootstub.
src/kernel/arch/riscv64/kernel-unstripped.elf: $(srcdir)/src/kernel/arch/riscv64/kernel.ld $(kernel-objs)
	@mkdir -p $(dir $@)
	@echo "LD      $@"
	$(Q)$(CC) $(kernel-cflags) $(kernel-ldflags) \
		-T $(srcdir)/src/kernel/arch/riscv64/kernel.ld -o $@ \
		$(kernel-objs)

# Split the kernel into the actual binary we want to load and its debug
# symbols. Using these does mean we won't get debug symbols for the bootstub,
# but since it gets loaded to lomem anyways... maybe we don't care about that.
src/kernel/arch/riscv64/kernel.elf: src/kernel/arch/riscv64/kernel-unstripped.elf
	@mkdir -p $(dir $@)
	@echo "STRIP   $@"
	$(Q)$(STRIP) -o $@ $<
src/kernel/kernel.sym: src/kernel/arch/riscv64/kernel-unstripped.elf
	@mkdir -p $(dir $@)
	@echo "STRIP   $@"
	$(Q)$(STRIP) --only-keep-debug -o $@ $<

# Generate the bootstub.
src/kernel/arch/riscv64/bootstub_generated.ld src/kernel/arch/riscv64/bootstub_generated.S &: $(srcdir)/src/kernel/arch/riscv64/generate_bootstub.py src/kernel/arch/riscv64/kernel.elf
	@mkdir -p $(dir $@)
	@echo "GEN     $@"
	$(Q)$(PYTHON3) $(srcdir)/src/kernel/arch/riscv64/generate_bootstub.py \
		src/kernel/arch/riscv64/kernel-unstripped.elf \
		src/kernel/arch/riscv64/bootstub_generated.ld \
		src/kernel/arch/riscv64/bootstub_generated.S

# Compile the bootstub.
src/kernel/arch/riscv64/bootstub_generated.o: src/kernel/arch/riscv64/bootstub_generated.S
	@mkdir -p $(dir $@)
	@echo "AS      $@"
	$(Q)$(CC) -c -o $@ $(kernel-cflags) $<

# Link the bootstub to produce the bootable ELF. The bootstub includes the
# kernel, so we don't need to somehow include it.
src/kernel/kernel.elf: $(srcdir)/src/kernel/arch/riscv64/bootstub.ld src/kernel/arch/riscv64/bootstub_generated.ld src/kernel/arch/riscv64/bootstub.o src/kernel/arch/riscv64/bootstub_generated.o
	@mkdir -p $(dir $@)
	@echo "LD      $@"
	$(Q)$(CC) $(kernel-cflags) $(kernel-ldflags) \
		-T $(srcdir)/src/kernel/arch/riscv64/bootstub.ld -o $@ \
		-s -Wl,--no-gc-sections \
		src/kernel/arch/riscv64/bootstub.o \
		src/kernel/arch/riscv64/bootstub_generated.o
