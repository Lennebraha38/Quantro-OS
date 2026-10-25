#ifndef QUANTRO_PROC_H
#define QUANTRO_PROC_H

#include "types.h"
#include "traps.h"

#define MAX_TASKS 16
#define TASK_STACK_SIZE 16384

#define TASK_FREE    0
#define TASK_RUNNING 1
#define TASK_READY   2
#define TASK_SLEEP   3
#define TASK_ZOMBIE  4

typedef void (*task_entry_t)(void);

typedef struct {
    int           id;
    int           state;
    int           priority;
    uint32_t      ticks;
    char          name[24];
    cpu_context_t context;
    uint8_t       stack[TASK_STACK_SIZE] __attribute__((aligned(16)));
} task_t;

void proc_init(void);
void proc_tick(void);
int  proc_spawn(const char *name);
int  proc_create(const char *name, task_entry_t entry);
int  proc_kill(int pid);
void proc_list(void);
int  proc_count(void);
const task_t *proc_get(int index);

#endif /* QUANTRO_PROC_H */
