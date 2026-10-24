#ifndef NET_H
#define NET_H
#include "types.h"
void net_init(void);
int net_send(const void *pkt, uint32_t len);
int net_recv(void *pkt, uint32_t *len);
#endif
