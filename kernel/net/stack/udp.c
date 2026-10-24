#include "udp.h"
#include "uart.h"
void udp_init(void){ uart_puts("[net:udp] init (stub)\r\n"); }
int udp_send(uint32_t ip, uint16_t port, const void *data, uint32_t len){ (void)ip;(void)port;(void)data;(void)len; return 0; /* stub */ }
