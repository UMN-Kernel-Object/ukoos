# Include config.mak, and make sure that it is present.
-include config.mak
ifndef config_mak_present
$(error The build was not configured; run ./configure)
endif

# Turn off some built-in behavior we don't want.
MAKEFLAGS += -rR

# Helpers.
ifeq ($(V),1)
  Q =
else
  Q = @
endif
define defcleanable_one
clean::
	@if [[ -e $(1) ]]; then echo "CLEAN   $(1)"; rm $(1); fi
endef
define defcleanable
$(foreach arg,$(1),$(eval $(call defcleanable_one,$(arg))))
endef
define compute_component_variables
$(1)-deps = $(addprefix $($(1)-dir)/,$(addsuffix .d,$($(1)-objs-asm) $($(1)-objs-c)))
$(1)-objs = $(addprefix $($(1)-dir)/,$(addsuffix .o,$($(1)-objs-asm) $($(1)-objs-c)))
$(1)-srcs-asm = $(addprefix $(srcdir)/$($(1)-dir)/,$(addsuffix .S,$($(1)-objs-asm)))
$(1)-srcs-c = $(addprefix $(srcdir)/$($(1)-dir)/,$(addsuffix .c,$($(1)-objs-c)))
$(1)-objdirs = $$(sort $$(foreach obj,$$($(1)-objs),$$(dir $$(obj))))

$(1)-cflags ?=
$(1)-ldflags ?=
endef

# Information about the build to perform.
components = kernel
kernel-cflags = $(CFLAGS) \
	-ffile-prefix-map=$(srcdir)= \
	-ffreestanding \
	-fno-builtin-main \
	-fwrapv \
	-isystem $(srcdir)/src/kernel/include \
	-isystem $(srcdir)/src/kernel/arch/$(arch)/include \
	-nostdlib \
	-std=c2x
kernel-cflags += -fdata-sections -ffunction-sections
kernel-ldflags += -Wl,--gc-sections
kernel-dir = src/kernel
kernel-objs-asm =
kernel-objs-c = devicetree main panic print selftest symbolicate
kernel-objs-c += builtins/bzero builtins/explicit_bzero builtins/memcpy builtins/memcmp builtins/memset builtins/strlen
kernel-objs-c += crypto/subtle/rfc7539
kernel-objs-c += mm/physical_alloc mm/virtual_alloc
include $(srcdir)/src/kernel/arch/$(arch)/include.mak

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

define common_rules_for_dir
$(2)%.o: $(srcdir)/$(2)%.c
	@mkdir -p $$(dir $$@)
	@echo "CC      $$@"
	$(Q)$(CC) -c -o $$@ $$($(1)-cflags) $$<

$(2)%.o: $(srcdir)/$(2)%.S
	@mkdir -p $$(dir $$@)
	@echo "AS      $$@"
	$(Q)$(CC) -c -o $$@ $$($(1)-cflags) $$<

# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html
$(2)%.d: $(srcdir)/$(2)%.c
	@mkdir -p $$(dir $$@)
	@set -e; rm -f $$@; \
	trap 'rm -f $$@.$$$$$$$$' EXIT; \
	$(CC) -M $$($(1)-cflags) $$< > $$@.$$$$$$$$; \
	sed 's,$$(notdir $$(@:%.d=%.o))[ :]*,$$(@:%.d=%.o) $$@ : ,g' < $$@.$$$$$$$$ > $$@
$(2)%.d: $(srcdir)/$(2)%.S
	@mkdir -p $$(dir $$@)
	@set -e; rm -f $$@; \
	trap 'rm -f $$@.$$$$$$$$' EXIT; \
	$(CC) -M $$($(1)-cflags) $$< > $$@.$$$$$$$$; \
	sed 's,$$(notdir $$(@:%.d=%.o))[ :]*,$$(@:%.d=%.o) $$@ : ,g' < $$@.$$$$$$$$ > $$@
endef
define common_rules_for_component
$(call defcleanable,$($(1)-deps))
$(call defcleanable,$($(1)-objs))
$(foreach dir,$($(1)-objdirs),$(eval $(call common_rules_for_dir,$(1),$(dir))))
ifeq (,$(filter clean,$(MAKECMDGOALS)))
-include $$($(1)-deps)
endif
endef
$(foreach component,$(components),$(eval $(call common_rules_for_component,$(component))))
