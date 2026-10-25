/*
 * Quantro OS - AArch64 Exception Vector Tablosu (VBAR_EL1)
 */

.section .text
.align 11 /* 2048 byte hizalama */
.global exception_vectors
exception_vectors:

/* 1. Current EL with SP0 */
.align 7
curr_el_sp0_sync:
    b trap_sync
.align 7
curr_el_sp0_irq:
    b trap_irq
.align 7
curr_el_sp0_fiq:
    b trap_fiq
.align 7
curr_el_sp0_serror:
    b trap_serror

/* 2. Current EL with SPx */
.align 7
curr_el_spx_sync:
    b trap_sync
.align 7
curr_el_spx_irq:
    b trap_irq
.align 7
curr_el_spx_fiq:
    b trap_fiq
.align 7
curr_el_spx_serror:
    b trap_serror

/* 3. Lower EL using AArch64 */
.align 7
lower_el_a64_sync:
    b trap_sync
.align 7
lower_el_a64_irq:
    b trap_irq
.align 7
lower_el_a64_fiq:
    b trap_fiq
.align 7
lower_el_a64_serror:
    b trap_serror

/* 4. Lower EL using AArch32 */
.align 7
lower_el_a32_sync:
    b trap_sync
.align 7
lower_el_a32_irq:
    b trap_irq
.align 7
lower_el_a32_fiq:
    b trap_fiq
.align 7
lower_el_a32_serror:
    b trap_serror

/* Ortak Trap Rutini */
.macro SAVE_ALL
    sub sp, sp, #256
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    mrs x21, elr_el1
    mrs x22, spsr_el1
    stp x30, x21, [sp, #240]
.endm

.macro RESTORE_ALL
    ldp x30, x21, [sp, #240]
    msr elr_el1, x21
    ldp x28, x29, [sp, #224]
    ldp x26, x27, [sp, #208]
    ldp x24, x25, [sp, #192]
    ldp x22, x23, [sp, #176]
    ldp x20, x21, [sp, #160]
    ldp x18, x19, [sp, #144]
    ldp x16, x17, [sp, #128]
    ldp x14, x15, [sp, #112]
    ldp x12, x13, [sp, #96]
    ldp x10, x11, [sp, #80]
    ldp x8, x9, [sp, #64]
    ldp x6, x7, [sp, #48]
    ldp x4, x5, [sp, #32]
    ldp x2, x3, [sp, #16]
    ldp x0, x1, [sp, #0]
    add sp, sp, #256
.endm

trap_sync:
    SAVE_ALL
    mov x0, sp
    bl handle_sync_exception
    RESTORE_ALL
    eret

trap_irq:
    SAVE_ALL
    mov x0, sp
    bl handle_irq_exception
    RESTORE_ALL
    eret

trap_fiq:
    SAVE_ALL
    mov x0, sp
    bl handle_fiq_exception
    RESTORE_ALL
    eret

trap_serror:
    SAVE_ALL
    mov x0, sp
    bl handle_serror_exception
    RESTORE_ALL
    eret
