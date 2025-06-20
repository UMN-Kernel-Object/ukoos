kernel-cflags += -mabi=lp64 -march=rv64imac_zicsr_zicntr_zihpm_zba_zbb_zbs_zihintpause_zicbom_zicbop_zicboz -mcmodel=medany
kernel-cflags += -Wconversion
kernel-objs-c += arch/riscv64/halt arch/riscv64/paging arch/riscv64/sbi

$(eval $(call compute_component_variables,kernel))

all:: src/kernel/kernel.elf src/kernel/kernel.sym
install:: src/kernel/kernel.elf src/kernel/kernel.sym
	install -DT src/kernel/kernel.elf $(DESTDIR)$(prefix)/sys/kernel.elf
	install -DT src/kernel/kernel.sym $(DESTDIR)$(prefix)/sys/kernel.sym

$(call defcleanable, \
	src/kernel/arch/riscv64/bootstub.d \
	src/kernel/arch/riscv64/bootstub.o \
	src/kernel/arch/riscv64/bootstub_generated.o \
	src/kernel/arch/riscv64/bootstub_generated.S \
	src/kernel/arch/riscv64/kernel-unstripped.elf \
	src/kernel/arch/riscv64/kernel.elf \
	src/kernel/kernel.elf \
	src/kernel/kernel.sym)

# Targets for booting and debugging the kernel.
gdb: src/kernel/kernel.sym
	gdb src/kernel/kernel.sym \
		-ex "layout src" \
		-ex "focus cmd" \
		-ex "target remote :1234" \
		-ex "break main" \
		-ex "break _panic_halt" \
		-ex "continue"
gdb_bootstub: src/kernel/kernel.sym
	gdb src/kernel/kernel.sym \
		-ex "layout asm" \
		-ex "layout regs" \
		-ex "focus cmd" \
		-ex "target remote :1234" \
		-ex "break *0x80080000" \
		-ex "continue"
qemu: src/kernel/kernel.elf
	qemu-system-riscv64 \
		--machine virt \
		--cpu rva22s64 \
		--smp 1 \
		-m 1G \
		-nographic \
		-kernel src/kernel/kernel.elf \
		$(QEMUFLAGS)
.PHONY: gdb gdb_bootstub qemu

# Link the kernel. This kernel will have debug symbols, and not actually be
# bootable -- it depends on being loaded by and having the boot environment set
# up by the bootstub.
src/kernel/arch/riscv64/kernel-unstripped.elf: $(srcdir)/src/kernel/arch/riscv64/kernel.ld $(kernel-objs)
	@mkdir -p $(dir $@)
	@echo "LD      $(basename $@)"
	$(Q)$(CC) $(kernel-cflags) $(kernel-ldflags) \
		-T $(srcdir)/src/kernel/arch/riscv64/kernel.ld -o $@ \
		$(kernel-objs) -lgcc

# Split the kernel into the actual binary we want to load and its debug
# symbols. Using these does mean we won't get debug symbols for the bootstub,
# but since it gets loaded to lomem anyways... maybe we don't care about that.
src/kernel/arch/riscv64/kernel.elf: src/kernel/arch/riscv64/kernel-unstripped.elf
	@mkdir -p $(dir $@)
	@echo "STRIP   $(basename $@)"
	$(Q)$(STRIP) -o $@ $<
src/kernel/kernel.sym: src/kernel/arch/riscv64/kernel-unstripped.elf
	@mkdir -p $(dir $@)
	@echo "STRIP   $(basename $@)"
	$(Q)$(STRIP) --only-keep-debug -o $@ $<

# Generate the bootstub.
src/kernel/arch/riscv64/bootstub_generated.S: $(srcdir)/src/kernel/arch/riscv64/generate_bootstub.py src/kernel/arch/riscv64/kernel.elf
	@mkdir -p $(dir $@)
	@echo "GEN     $(basename $@)"
	$(Q)python3 $(srcdir)/src/kernel/arch/riscv64/generate_bootstub.py src/kernel/arch/riscv64/kernel-unstripped.elf $@

# Compile the bootstub.
src/kernel/arch/riscv64/bootstub_generated.o: src/kernel/arch/riscv64/bootstub_generated.S
	@mkdir -p $(dir $@)
	@echo "AS      $(basename $@)"
	$(Q)$(CC) -c -o $@ $(kernel-cflags) $<

# Link the bootstub to produce the bootable ELF. The bootstub includes the
# kernel, so we don't need to somehow include it.
src/kernel/kernel.elf: $(srcdir)/src/kernel/arch/riscv64/bootstub.ld src/kernel/arch/riscv64/bootstub.o src/kernel/arch/riscv64/bootstub_generated.o
	@mkdir -p $(dir $@)
	@echo "LD      $(basename $@)"
	$(Q)$(CC) $(kernel-cflags) $(kernel-ldflags) \
		-T $(srcdir)/src/kernel/arch/riscv64/bootstub.ld -o $@ -Wl,--no-warn-rwx-segments \
		src/kernel/arch/riscv64/bootstub.o \
		src/kernel/arch/riscv64/bootstub_generated.o
