#include "tcp.h"
#include "uart.h"
/* TCP state machine iskeleti */
void tcp_init(void){ uart_puts("[net:tcp] init (stub, state machine ready)\r\n"); }
int tcp_connect(uint32_t ip, uint16_t port){ (void)ip;(void)port; return 0; /* stub */ }
int tcp_send(int sock, const void *data, uint32_t len){ (void)sock;(void)data;(void)len; return 0; /* stub */ }
int tcp_recv(int sock, void *buf, uint32_t len){ (void)sock;(void)buf;(void)len; return 0; /* stub */ }
