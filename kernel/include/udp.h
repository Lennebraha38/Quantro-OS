#ifndef UDP_H
#define UDP_H
#include "types.h"
void udp_init(void);
int udp_send(uint32_t ip, uint16_t port, const void *data, uint32_t len);
#endif
