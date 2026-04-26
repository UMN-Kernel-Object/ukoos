/*
 * SPDX-FileCopyrightText: 2026 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * Switches tasks.
 */
.section .text
.p2align 2
.global task_switch
.type task_switch, @function
task_switch:
  // Get the current hart and task.
  csrr a1, sscratch
  ld a2, 0(a1) // a2 = hart
  ld a3, 8(a1) // a3 = task

  // Get the current hart's hart_group.
  ld a4, 40(a2) // a4 = hart_group

  // Get the current task's hart_group.
  ld a5, 24(a3) // a5 = hart_group

  // Check that the hart_groups match. If so, we know it's a riscv_hart_group.
  // If not, die -- our invariants are being messed with.
  //
  // TODO: We're assuming every hart in the system is riscv64. This is a safe
  // assumption for now, but it could be nice to make this code path correct if
  // that changes in the future.
  bne a4, a5, 1f

  // Get the new hart's hart group and check that it matches too.
  ld a5, 24(a0) // a5 = hart_group
  bne a4, a5, 1f

  // Get the current task's flags, and check that they're either
  // TASK_STATE_RUNNABLE or TASK_STATE_BLOCKED. (The caller should have set
  // them already.)
  lw a2, 16(a3) // a2 = old_task->flags
  and a2, a2, 0b10
  beqz a2, 1f

  // Get the new task's flags, and check that they're TASK_STATE_RUNNING.
  lw a2, 16(a0) // a2 = new_task->flags
  and a2, a2, 0b11
  li t0, 1
  bne a2, t0, 1f

  // Save the new task to the hart locals.
  sd a0, 8(a1)

  // Get the f_off and f_len. Jump to the appropriate place based on the f_len,
  // to swap the callee-save floating-point registers.
  ld a5, 16(a4) // a5 = f_off
  add a6, a0, a5 // a6 = &new_task->register_save[f_off] - 40
  add a5, a3, a5 // a5 = &old_task->register_save[f_off] - 40
  ld t0, 40(a4) // t0 = f_len
  li t1, 0
  beq t0, t1, 2f
  li t1, 128
  beq t0, t1, 3f
  li t1, 256
  beq t0, t1, 4f
  li t1, 512
  beq t0, t1, 5f
  j 1f

5: // FDQ extensions.
  ebreak // TODO

4: // FD extensions.
  fsd fs0, 104(a5)
  fsd fs1, 112(a5)
  fsd fs2, 184(a5)
  fsd fs3, 192(a5)
  fsd fs4, 200(a5)
  fsd fs5, 208(a5)
  fsd fs6, 216(a5)
  fsd fs7, 224(a5)
  fsd fs8, 232(a5)
  fsd fs9, 240(a5)
  fsd fs10, 248(a5)
  fsd fs11, 256(a5)

  fld ft0, 40(a6)
  fld ft1, 48(a6)
  fld ft2, 56(a6)
  fld ft3, 64(a6)
  fld ft4, 72(a6)
  fld ft5, 80(a6)
  fld ft6, 88(a6)
  fld ft7, 96(a6)
  fld fs0, 104(a6)
  fld fs1, 112(a6)
  fld fa0, 120(a6)
  fld fa1, 128(a6)
  fld fa2, 136(a6)
  fld fa3, 144(a6)
  fld fa4, 152(a6)
  fld fa5, 160(a6)
  fld fa6, 168(a6)
  fld fa7, 176(a6)
  fld fs2, 184(a6)
  fld fs3, 192(a6)
  fld fs4, 200(a6)
  fld fs5, 208(a6)
  fld fs6, 216(a6)
  fld fs7, 224(a6)
  fld fs8, 232(a6)
  fld fs9, 240(a6)
  fld fs10, 248(a6)
  fld fs11, 256(a6)
  fld ft8, 264(a6)
  fld ft9, 272(a6)
  fld ft10, 280(a6)
  fld ft11, 288(a6)

  j 2f

3: // F extension.
  ebreak // TODO

2: // No F extension (or done swapping F registers).

  // Get the x_off and x_len. Check that x_len is what we expect (264). If not,
  // die.
  ld a5, 8(a4) // a5 = x_off
  add t6, a0, a5 // t6 = &new_task->register_save[x_off] - 40
  add a5, a3, a5 // a5 = &old_task->register_save[x_off] - 40
  ld t0, 32(a4) // t0 = x_len
  li t1, 264
  bne t0, t1, 1f

  // Save task_resume as the PC for the old task.
  la t0, task_resume
  sd t0, 40(a5)

  // Save SSTATUS to the old task, setting its SPP bit so that we return to
  // supervisor mode.
  csrr t0, sstatus
  ori t0, t0, 1<<8
  sd t0, 296(a5)

  // Load SEPC and SSTATUS from the new task.
  ld t0, 40(t6)
  csrw sepc, t0
  ld t0, 296(t6)
  csrw sstatus, t0

  // Swap the callee-save GPRs and the link address between the old and new tasks.
  sd ra, 48(a5)
  sd sp, 56(a5)
  sd gp, 64(a5)
  sd tp, 72(a5)
  sd s0, 104(a5)
  sd s1, 112(a5)
  sd s2, 184(a5)
  sd s3, 192(a5)
  sd s4, 200(a5)
  sd s5, 208(a5)
  sd s6, 216(a5)
  sd s7, 224(a5)
  sd s8, 232(a5)
  sd s9, 240(a5)
  sd s10, 248(a5)
  sd s11, 256(a5)

  ld ra, 48(t6)
  ld sp, 56(t6)
  ld gp, 64(t6)
  ld tp, 72(t6)
  ld t0, 80(t6)
  ld t1, 88(t6)
  ld t2, 96(t6)
  ld s0, 104(t6)
  ld s1, 112(t6)
  ld a0, 120(t6)
  ld a1, 128(t6)
  ld a2, 136(t6)
  ld a3, 144(t6)
  ld a4, 152(t6)
  ld a5, 160(t6)
  ld a6, 168(t6)
  ld a7, 176(t6)
  ld s2, 184(t6)
  ld s3, 192(t6)
  ld s4, 200(t6)
  ld s5, 208(t6)
  ld s6, 216(t6)
  ld s7, 224(t6)
  ld s8, 232(t6)
  ld s9, 240(t6)
  ld s10, 248(t6)
  ld s11, 256(t6)
  ld t3, 264(t6)
  ld t4, 272(t6)
  ld t5, 280(t6)
  ld t6, 288(t6)

  // Jump back to SEPC.
  sret

1:
	ebreak

/**
 * The stub code used to resume from a task.
 */
.section .text
.p2align 2
.type task_resume, @function
task_resume:
	ret
