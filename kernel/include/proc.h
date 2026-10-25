#ifndef QUANTRO_PROC_H
#define QUANTRO_PROC_H

#include "types.h"

#define MAX_TASKS 16

#define TASK_FREE    0
#define TASK_RUNNING 1
#define TASK_READY   2
#define TASK_SLEEP   3
#define TASK_ZOMBIE  4

typedef struct {
    int      id;
    int      state;
    int      priority; /* 0-3: Normal, High, Realtime */
    uint32_t ticks;
    char     name[24];
} task_t;

void proc_init(void);
void proc_tick(void);
int  proc_spawn(const char *name);
int  proc_kill(int pid);
void proc_list(void);
int  proc_count(void);
const task_t *proc_get(int index);

#endif /* QUANTRO_PROC_H */
