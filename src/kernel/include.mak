# SPDX-FileCopyrightText: 2025 ukoOS Contributors
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
kernel-objs-asm =
kernel-objs-c = devicetree init main panic print random selftest swar_test symbolicate
kernel-objs-c += builtins/bzero builtins/explicit_bzero builtins/memcpy builtins/memcmp builtins/memset builtins/strcmp builtins/strlen
kernel-objs-c += crypto/subtle/rfc7539 crypto/subtle/rfc7693
kernel-objs-c += mm/alloc mm/alloc/block mm/alloc/heap mm/alloc/init mm/alloc/page mm/alloc/segment mm/physical_alloc mm/virtual_alloc
include $(srcdir)/src/kernel/arch/$(arch)/include.mak
