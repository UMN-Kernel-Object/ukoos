# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

kernel-cflags += -march=rv64imafdcbv_zicsr_zicntr_zihpm_ziccif_ziccrse_ziccamoa_zicclsm_zic64b_za64rs_zihintpause_zba_zbb_zbs_zicbom_zicbop_zicboz_zfhmin_zkt_zvfhmin_zvbb_zvkt_zihintntl_zicond_zimop_zcmop_zcb_zfa_zawrs_svpbmt_svinval_svnapot_sstc_sscofpmf_zifencei
target-qemuflags = --machine virt --cpu rva23s64 --smp 2 -m 1G --netdev user,id=net0 -device rtl8139,netdev=net0,bus=pcie.0
