# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

components += kernel
kernel-cflags = $(CFLAGS) \
	-ffile-prefix-map=$(srcdir)= \
	-ffreestanding \
	-fno-builtin-main \
	-fstack-check \
	-fwrapv \
	-isystem $(srcdir)/src/kernel/include \
	-isystem $(srcdir)/src/kernel/arch/$(arch)/include \
	-nostdlib \
	-std=c23 \
	-Walloca \
	-Wconversion \
	-Wsystem-headers \
	-Wvla \
	-Wwrite-strings
kernel-cflags += -fdata-sections -ffunction-sections
kernel-ldflags += -Wl,--gc-sections
kernel-dir = src/kernel
kernel-objs-c += builtins/bzero
kernel-objs-c += builtins/explicit_bzero
kernel-objs-c += builtins/memcmp
kernel-objs-c += builtins/memcpy
kernel-objs-c += builtins/memset
kernel-objs-c += builtins/strcmp
kernel-objs-c += builtins/strlen
kernel-objs-c += builtins/strnlen
kernel-objs-c += crypto/subtle/rfc7539
kernel-objs-c += crypto/subtle/rfc7693
kernel-objs-c += device
kernel-objs-c += devices/pci
kernel-objs-c += devices/uart
kernel-objs-c += devices/netdev
kernel-objs-c += devicetree
kernel-objs-c += init
kernel-objs-c += main
kernel-objs-c += mm/alloc
kernel-objs-c += mm/alloc/block
kernel-objs-c += mm/alloc/heap
kernel-objs-c += mm/alloc/init
kernel-objs-c += mm/alloc/page
kernel-objs-c += mm/alloc/segment
kernel-objs-c += mm/physical_alloc
kernel-objs-c += mm/virtual_alloc
kernel-objs-c += panic
kernel-objs-c += print
kernel-objs-c += random
kernel-objs-c += selftest
kernel-objs-c += swar_test
kernel-objs-c += symbolicate
kernel-objs-c += net/ipv6
include $(srcdir)/src/kernel/drivers/include.mak
include $(srcdir)/src/kernel/net/include.mak

# The architecture-specific file needs to be last, since it calls
# compute_component_variables for the kernel.
include $(srcdir)/src/kernel/arch/$(arch)/include.mak
