/*
 * Benim-OS Pro - AArch64 generic timer
 *
 * CNTFRQ_EL0 sistem sayaci frekansini, CNTPCT_EL0 ise sayac
 * degerini verir. Kesme kullanmiyoruz; ana dongu yoklama yapar.
 */
#include "kernel.h"

static uint64_t counter_frequency = 62500000ULL;

void timer_init(void)
{
    uint64_t freq;

    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));

    if (freq)
        counter_frequency = freq;
}

uint64_t timer_frequency(void)
{
    return counter_frequency;
}

uint64_t timer_ticks(void)
{
    uint64_t ticks;

    __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(ticks));

    return ticks;
}
