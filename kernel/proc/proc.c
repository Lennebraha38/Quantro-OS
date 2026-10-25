#include "proc.h"
#include "uart.h"
#include "kernel.h"

static task_t tasks[MAX_TASKS];
static int task_count = 0;
static int current = 0;

void proc_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].id = i;
        tasks[i].state = TASK_FREE;
        tasks[i].priority = 1;
        tasks[i].ticks = 0;
        tasks[i].name[0] = '\0';
    }
    task_count = 0;

    /* Windows 11 mimari surec agaci */
    proc_spawn("System");
    proc_spawn("smss.exe");
    proc_spawn("csrss.exe");
    proc_spawn("wininit.exe");
    proc_spawn("services.exe");
    proc_spawn("lsass.exe");
    proc_spawn("dwm.exe");
    proc_spawn("explorer.exe");

    uart_puts("[proc] Windows 11 Scheduler / NT Process Manager hazir (Round-Robin preemptive)\r\n");
}

void proc_tick(void)
{
    if (task_count == 0)
        return;

    tasks[current].ticks++;
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
}

int proc_spawn(const char *name)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_FREE || tasks[i].state == TASK_ZOMBIE) {
            tasks[i].id = i;
            tasks[i].state = TASK_READY;
            tasks[i].priority = 1;
            tasks[i].ticks = 0;
            strncpy(tasks[i].name, name, sizeof(tasks[i].name) - 1);
            tasks[i].name[sizeof(tasks[i].name) - 1] = '\0';
            if (i >= task_count)
                task_count = i + 1;
            return i;
        }
    }
    return -1;
}

int proc_kill(int pid)
{
    if (pid <= 0 || pid >= MAX_TASKS)
        return -1; /* PID 0 (System) korunuyor */

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
    uart_puts("\r\n Image Name                     PID    Durum       Oncelik   Calisma Zamani\r\n");
    uart_puts(" ==========================================================================\r\n");

    const char *state_str[] = { "FREE   ", "RUNNING", "READY  ", "SLEEP  ", "ZOMBIE " };

    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state != TASK_FREE) {
            const char *st = (tasks[i].state >= 0 && tasks[i].state <= 4) ?
                              state_str[tasks[i].state] : "UNKNOWN";
            uart_printf(" %-28s %3d    %-10s %7d   %8u ticks\r\n",
                        tasks[i].name,
                        tasks[i].id,
                        st,
                        tasks[i].priority,
                        tasks[i].ticks);
        }
    }
    uart_puts(" --------------------------------------------------------------------------\r\n");
}
