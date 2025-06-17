# Build options, configurable with config.mak.
CC = riscv64-none-elf-gcc
STRIP = riscv64-none-elf-strip
CFLAGS = -g -O3 -Wall -Werror
LDFLAGS =
prefix = /
-include config.mak
ifndef arch
$(error The build was not configured; run ./configure)
endif
ifndef srcdir
$(error The build was not configured; run ./configure)
endif

# Information about the build to perform.
components = kernel
kernel-cflags = $(CFLAGS) \
	-ffreestanding \
	-fmacro-prefix-map=$(srcdir)/src/kernel/= \
	-isystem $(srcdir)/src/kernel/include \
	-isystem $(srcdir)/src/kernel/arch/$(arch)/include \
	-nostdlib \
	-std=c2x
kernel-dir = src/kernel
kernel-objs-asm =
kernel-objs-c = main

# Computed values; not intended to be user-modifiable.
define compute_component_variables
$(1)-deps = $(addprefix $($(1)-dir)/,$(addsuffix .d,$($(1)-objs-asm) $($(1)-objs-c)))
$(1)-objs = $(addprefix $($(1)-dir)/,$(addsuffix .o,$($(1)-objs-asm) $($(1)-objs-c)))
$(1)-srcs-asm = $(addprefix $(srcdir)/$($(1)-dir)/,$(addsuffix .S,$($(1)-objs-asm)))
$(1)-srcs-c = $(addprefix $(srcdir)/$($(1)-dir)/,$(addsuffix .c,$($(1)-objs-c)))

$(1)-cflags ?=
$(1)-ldflags ?=
endef
$(foreach component,$(components),$(eval $(call compute_component_variables,$(component))))

# Helper functions.
define defcleanable_one
clean::
	@if [[ -e $(1) ]]; then echo "CLEAN   $(1)"; rm $(1); fi
endef
define defcleanable
$(foreach arg,$(1),$(eval $(call defcleanable_one,$(arg))))
endef

# Common rules.
all::
$(call defcleanable,compile_commands.json)
distclean: clean
	@if [[ -e config.mak ]]; then echo "CLEAN config.mak"; rm config.mak; fi
install::
watch:
	watchexec \
		--clear clear \
		--restart \
		--watch $(srcdir)/src \
		-- $(MAKE) $(filter-out watch,$(MAKECMDGOALS))
.PHONY: all clean install watch
.SUFFIXES:

define common_rules
$(call defcleanable,$($(1)-deps))
$(call defcleanable,$($(1)-objs))

$($(1)-dir)/%.o: $(srcdir)/$($(1)-dir)/%.c
	@mkdir -p $$(dir $$@)
	@echo "CC      $$(basename $$@)" && \
	$(CC) -c -o $$@ $$($(1)-cflags) $$<

$($(1)-dir)/%.o: $(srcdir)/$($(1)-dir)/%.S
	@mkdir -p $$(dir $$@)
	@echo "AS      $$(basename $$@)" && \
	$(CC) -c -o $$@ $$($(1)-cflags) $$<

# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html
$($(1)-dir)/%.d: $(srcdir)/$($(1)-dir)/%.c
	@mkdir -p $$(dir $$@)
	@set -e; rm -f $$@; \
	trap 'rm -f $$@.$$$$$$$$' EXIT; \
	$(CC) -M $$($(1)-cflags) $$< > $$@.$$$$$$$$; \
	sed 's,$$(notdir $$(@:%.d=%.o))[ :]*,$$(@:%.d=%.o) $$@ : ,g' < $$@.$$$$$$$$ > $$@
$($(1)-dir)/%.d: $(srcdir)/$($(1)-dir)/%.S
	@mkdir -p $$(dir $$@)
	@set -e; rm -f $$@; \
	trap 'rm -f $$@.$$$$$$$$' EXIT; \
	$(CC) -M $$($(1)-cflags) $$< > $$@.$$$$$$$$; \
	sed 's,$$(notdir $$(@:%.d=%.o))[ :]*,$$(@:%.d=%.o) $$@ : ,g' < $$@.$$$$$$$$ > $$@
ifeq (,$(filter clean,$(MAKECMDGOALS)))
-include $$($(1)-deps)
endif
endef
$(foreach component,$(components),$(eval $(call common_rules,$(component))))

# src/kernel
include $(srcdir)/src/kernel/arch/$(arch)/include.mak
