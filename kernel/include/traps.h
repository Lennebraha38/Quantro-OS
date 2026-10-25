#ifndef QUANTRO_TRAPS_H
#define QUANTRO_TRAPS_H

#include "types.h"

typedef struct {
    uint64_t regs[31]; /* x0 - x30 */
    uint64_t elr_el1;
    uint64_t spsr_el1;
} trap_frame_t;

typedef struct {
    uint64_t x19, x20;
    uint64_t x21, x22;
    uint64_t x23, x24;
    uint64_t x25, x26;
    uint64_t x27, x28;
    uint64_t fp, lr;
    uint64_t sp;
} cpu_context_t;

void traps_init(void);
void handle_sync_exception(trap_frame_t *tf);
void handle_irq_exception(trap_frame_t *tf);
void handle_fiq_exception(trap_frame_t *tf);
void handle_serror_exception(trap_frame_t *tf);

extern void cpu_switch_to(cpu_context_t *prev, cpu_context_t *next);

#endif /* QUANTRO_TRAPS_H */
