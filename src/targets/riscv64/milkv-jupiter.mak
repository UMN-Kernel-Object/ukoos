# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

kernel-cflags += -march=rv64imafdcbv_zicsr_zicntr_zihpm_zba_zbb_zbs_zihintpause_zicbom_zicbop_zicboz_zifencei
target-qemuflags = --machine virt --cpu rva22s64 --smp 8 -m 4G
