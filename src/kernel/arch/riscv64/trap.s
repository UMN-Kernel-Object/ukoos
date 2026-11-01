  csrrw sscratch, ra
  sd sp, 4(ra)
  sd gp, 8(ra)
  sd tp, 12(ra)
  sd t0, 16(ra)
  sd t1, 20(ra)
  sd t2, 24(ra)
  sd s0, 28(ra)
  sd s1, 32(ra)
  sd a0, 36(ra)
  sd a1, 40(ra)
  sd a2, 44(ra)
  sd a3, 48(ra)
  sd a4, 52(ra)
  sd a5, 56(ra)
  sd a6, 60(ra)
  sd a7, 64(ra)
  sd s2, 68(ra)
  sd s3, 72(ra)
  sd s4, 76(ra)
  sd s5, 80(ra)
  sd s6, 84(ra)
  sd s7, 88(ra)
  sd s8, 92(ra)
  sd s9, 96(ra)
  sd s10, 100(ra)
  sd s11, 104(ra)
  sd t3, 108(ra)
  sd t4, 112(ra)
  sd t5, 116(ra)
  sd t6, 120(ra)
  sd a0, ra
  csrrw sscratch, ra
  sd ra, (a0)

  ld sstatus, a0
  # SPP bit in sstatus reg to check if trap originated from usermode
  andi a0, a0, 1<<8 
  bne a0, zero, supervisor

usermode:
  # TODO

supervisor:
  call trap_handler
  # TODO: put back registers and anything else we need to do for going back to usermode
