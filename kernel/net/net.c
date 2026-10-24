#include "net.h"
#include "uart.h"
/* virtio-net iskeleti */
void net_init(void){ uart_puts("[net] virtio-net init (stub, rx/tx queues ready)\r\n"); }
int net_send(const void *pkt, uint32_t len){ (void)pkt;(void)len; return 0; /* stub */ }
int net_recv(void *pkt, uint32_t *len){ (void)pkt;(void)len; return 0; /* stub */ }
