# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

kernel-cflags += -march=rv64imafdc_xtheadba_xtheadbb_xtheadbs_xtheadcmo_xtheadcondmov_xtheadfmemidx_xtheadmac_xtheadmemidx_xtheadmempair_xtheadsync_zicsr_zicntr_zihpm -mtune=thead-c906
target-qemuflags = --machine virt --cpu thead-c906 --smp 1 -m 512M
