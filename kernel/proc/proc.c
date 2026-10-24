#include "proc.h"
#include "uart.h"
void proc_init(void){ uart_puts("[proc] scheduler init (round-robin stub)\r\n"); }
void proc_spawn(const char *name){ uart_printf("[proc] spawn %s (stub)\r\n", name); }
