#include "wm.h"
#include "uart.h"
void wm_init(void){ uart_puts("[wm] window manager + compositor init (stub)\r\n"); }
void wm_create_window(const char *title){ uart_printf("[wm] create window: %s (stub)\r\n", title); }
