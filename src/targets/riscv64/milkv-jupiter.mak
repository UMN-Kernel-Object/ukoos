kernel-cflags += -march=rv64imac_zicsr_zicntr_zihpm_zba_zbb_zbs_zihintpause_zicbom_zicbop_zicboz
target-qemuflags = --machine virt --cpu rva22s64 --smp 8
