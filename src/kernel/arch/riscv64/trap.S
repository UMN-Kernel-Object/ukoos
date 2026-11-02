  csrrw ra, sscratch, ra
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
  mv a0, ra
  csrrw ra, sscratch, ra
  sd ra, (a0)

  csrr a0, sstatus
  # SPP bit in sstatus reg to check if trap originated from usermode
  andi a0, a0, 1<<8 
  bne a0, zero, supervisor

usermode:
  # TODO
  j restore

supervisor:
  call trap_handler

restore:
  csrrw ra, sscratch, ra
  ld sp, 4(ra)
  ld gp, 8(ra)
  ld tp, 12(ra)
  ld t0, 16(ra)
  ld t1, 20(ra)
  ld t2, 24(ra)
  ld s0, 28(ra)
  ld s1, 32(ra)
  ld a0, 36(ra)
  ld a1, 40(ra)
  ld a2, 44(ra)
  ld a3, 48(ra)
  ld a4, 52(ra)
  ld a5, 56(ra)
  ld a6, 60(ra)
  ld a7, 64(ra)
  ld s2, 68(ra)
  ld s3, 72(ra)
  ld s4, 76(ra)
  ld s5, 80(ra)
  ld s6, 84(ra)
  ld s7, 88(ra)
  ld s8, 92(ra)
  ld s9, 96(ra)
  ld s10, 100(ra)
  ld s11, 104(ra)
  ld t3, 108(ra)
  ld t4, 112(ra)
  ld t5, 116(ra)
  ld t6, 120(ra)
  mv a0, ra
  csrrw ra, sscratch, ra
