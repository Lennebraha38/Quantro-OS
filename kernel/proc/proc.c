#include "proc.h"
#include "uart.h"
#include "kernel.h"
#include "traps.h"

static task_t tasks[MAX_TASKS];
static int task_count = 0;
static int current = 0;

static void default_task_idle(void)
{
    for (;;) {
        __asm__ volatile("wfi" ::: "memory");
    }
}

void proc_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].id = i;
        tasks[i].state = TASK_FREE;
        tasks[i].priority = 1;
        tasks[i].ticks = 0;
        tasks[i].name[0] = '\0';
        memset(&tasks[i].context, 0, sizeof(cpu_context_t));
    }
    task_count = 0;
    current = 0;

    /* Ana cekirdek gorevi (PID 0) */
    tasks[0].id = 0;
    tasks[0].state = TASK_RUNNING;
    tasks[0].priority = 0;
    strncpy(tasks[0].name, "System (NT Kernel)", 23);
    task_count = 1;

    /* Windows 11 mimari surecleri */
    proc_create("smss.exe", default_task_idle);
    proc_create("csrss.exe", default_task_idle);
    proc_create("wininit.exe", default_task_idle);
    proc_create("services.exe", default_task_idle);
    proc_create("lsass.exe", default_task_idle);
    proc_create("dwm.exe", default_task_idle);
    proc_create("explorer.exe", default_task_idle);

    uart_puts("[proc] Windows NT Preemptive Scheduler + Donanimsal Context Switch hazir\r\n");
}

int proc_create(const char *name, task_entry_t entry)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_FREE || tasks[i].state == TASK_ZOMBIE) {
            tasks[i].id = i;
            tasks[i].state = TASK_READY;
            tasks[i].priority = 1;
            tasks[i].ticks = 0;
            strncpy(tasks[i].name, name, sizeof(tasks[i].name) - 1);
            tasks[i].name[sizeof(tasks[i].name) - 1] = '\0';

            /* AArch64 stack ve context kurulumu */
            uintptr_t stack_top = (uintptr_t)(tasks[i].stack + TASK_STACK_SIZE);
            stack_top &= ~0xFULL; /* 16-byte hizalama */

            memset(&tasks[i].context, 0, sizeof(cpu_context_t));
            tasks[i].context.sp = stack_top;
            tasks[i].context.lr = (uint64_t)entry;

            if (i >= task_count)
                task_count = i + 1;

            return i;
        }
    }
    return -1;
}

int proc_spawn(const char *name)
{
    return proc_create(name, default_task_idle);
}

void proc_tick(void)
{
    if (task_count <= 1)
        return;

    tasks[current].ticks++;
    int prev_idx = current;

    if (tasks[current].state == TASK_RUNNING)
        tasks[current].state = TASK_READY;

    int next = (current + 1) % MAX_TASKS;
    for (int i = 0; i < MAX_TASKS; i++) {
        int idx = (next + i) % MAX_TASKS;
        if (tasks[idx].state == TASK_READY || tasks[idx].state == TASK_RUNNING) {
            current = idx;
            tasks[current].state = TASK_RUNNING;
            break;
        }
    }

    if (prev_idx != current) {
        /* Donanimsal Context Switch */
        cpu_switch_to(&tasks[prev_idx].context, &tasks[current].context);
    }
}

int proc_kill(int pid)
{
    if (pid <= 0 || pid >= MAX_TASKS)
        return -1;

    if (tasks[pid].state != TASK_FREE) {
        tasks[pid].state = TASK_ZOMBIE;
        return 0;
    }
    return -1;
}

int proc_count(void)
{
    return task_count;
}

const task_t *proc_get(int index)
{
    if (index >= 0 && index < MAX_TASKS)
        return &tasks[index];
    return 0;
}

void proc_list(void)
{
    uart_puts("\r\n Image Name                     PID    Durum       Oncelik   Stack Base      Ticks\r\n");
    uart_puts(" ====================================================================================\r\n");

    const char *state_str[] = { "FREE   ", "RUNNING", "READY  ", "SLEEP  ", "ZOMBIE " };

    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state != TASK_FREE) {
            const char *st = (tasks[i].state >= 0 && tasks[i].state <= 4) ?
                              state_str[tasks[i].state] : "UNKNOWN";
            uart_printf(" %-28s %3d    %-10s %7d   0x%p %8u\r\n",
                        tasks[i].name,
                        tasks[i].id,
                        st,
                        tasks[i].priority,
                        (void *)tasks[i].stack,
                        tasks[i].ticks);
        }
    }
    uart_puts(" ------------------------------------------------------------------------------------\r\n");
}
