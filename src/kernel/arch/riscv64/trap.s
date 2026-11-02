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
  j restore

supervisor:
  call trap_handler

restore:
  csrrw sscratch, ra
  sd 4(ra), sp
  sd 8(ra), gp
  sd 12(ra), tp
  sd 16(ra), t0
  sd 20(ra), t1
  sd 24(ra), t2
  sd 28(ra), s0
  sd 32(ra), s1
  sd 36(ra), a0
  sd 40(ra), a1
  sd 44(ra), a2
  sd 48(ra), a3
  sd 52(ra), a4
  sd 56(ra), a5
  sd 60(ra), a6
  sd 64(ra), a7
  sd 68(ra), s2
  sd 72(ra), s3
  sd 76(ra), s4
  sd 80(ra), s5
  sd 84(ra), s6
  sd 88(ra), s7
  sd 92(ra), s8
  sd 96(ra), s9
  sd 100(ra), s10
  sd 104(ra), s11
  sd 108(ra), t3
  sd 112(ra), t4
  sd 116(ra), t5
  sd 120(ra), t6
  csrrw sscratch, ra
