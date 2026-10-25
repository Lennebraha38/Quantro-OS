#include "traps.h"
#include "uart.h"
#include "kernel.h"
#include "proc.h"

extern uint8_t exception_vectors[];

void traps_init(void)
{
    /* VBAR_EL1 yazmacini vektor tablomuza ayarla */
    uint64_t vbar = (uint64_t)&exception_vectors;
    __asm__ volatile("msr vbar_el1, %0" :: "r"(vbar) : "memory");
    __asm__ volatile("isb" ::: "memory");

    uart_printf("[arch:aarch64] VBAR_EL1 Exception Vector Tablosu yuklendi: 0x%llx\r\n",
                (unsigned long long)vbar);
}

void handle_sync_exception(trap_frame_t *tf)
{
    uint64_t esr, far;
    __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));
    __asm__ volatile("mrs %0, far_el1" : "=r"(far));

    uint64_t ec = (esr >> 26) & 0x3F;

    uart_printf("\r\n[KERNEL TRAP] Senkron Hata (Synchronous Exception)!\r\n");
    uart_printf("  EC=0x%llx, ESR=0x%llx, FAR=0x%llx, PC=0x%llx, SPSR=0x%llx\r\n",
                (unsigned long long)ec,
                (unsigned long long)esr,
                (unsigned long long)far,
                (unsigned long long)tf->elr_el1,
                (unsigned long long)tf->spsr_el1);

    kernel_panic("UNHANDLED_PAGE_FAULT_OR_INSTRUCTION_ABORT");
}

void handle_irq_exception(trap_frame_t *tf)
{
    (void)tf;
    /* Timer veya Harici Aygit Kesmesi */
    proc_tick();
}

void handle_fiq_exception(trap_frame_t *tf)
{
    (void)tf;
    uart_puts("[irq:fiq] Fast Interrupt alindi.\r\n");
}

void handle_serror_exception(trap_frame_t *tf)
{
    (void)tf;
    kernel_panic("SYSTEM_ERROR_ABORT");
}
