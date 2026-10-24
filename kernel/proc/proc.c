#include "proc.h"
#include "uart.h"
#define MAX_TASKS 8
typedef struct { int id; int state; char name[16]; } task_t;
static task_t tasks[MAX_TASKS];
static int task_count=0;
static int current=0;
void proc_init(void){
    for(int i=0;i<MAX_TASKS;i++) tasks[i].state=0;
    uart_puts("[proc] round-robin scheduler ready (max 8 tasks)\r\n");
}
void proc_spawn(const char *name){
    if(task_count>=MAX_TASKS){ uart_puts("[proc] task table full\r\n"); return; }
    tasks[task_count].id=task_count;
    tasks[task_count].state=1;
    int i=0; while(name[i] && i<15){ tasks[task_count].name[i]=name[i]; i++; } tasks[task_count].name[i]='\0';
    uart_printf("[proc] spawned task %d: %s\r\n", task_count, tasks[task_count].name);
    task_count++;
}
