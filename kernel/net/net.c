#include "net.h"
#include "uart.h"
/* virtio-net iskeleti: MMIO + RX/TX virtqueue */
static int net_ready = 0;
void net_init(void){ net_ready=1; uart_puts("[net] virtio-net init (stub, queues ready)\r\n"); }
int net_send(const void *pkt, uint32_t len){ (void)pkt;(void)len; if(!net_ready) return -1; return 0; /* stub */ }
int net_recv(void *pkt, uint32_t *len){ (void)pkt;(void)len; if(!net_ready) return -1; return 0; /* stub */ }
/* ARP */
void arp_resolve(uint32_t ip){ (void)ip; uart_puts("[net:arp] resolve (stub)\r\n"); }
/* ICMP */
void icmp_echo(uint32_t ip){ (void)ip; uart_puts("[net:icmp] echo (stub)\r\n"); }
