kernel-cflags += -march=rv64imac_xtheadba_xtheadbb_xtheadbs_xtheadcmo_xtheadcondmov_xtheadfmemidx_xtheadmac_xtheadmemidx_xtheadmempair_xtheadsync_zicsr_zicntr_zihpm -mtune=thead-c906
target-qemuflags = --machine virt --cpu thead-c906 --smp 1
