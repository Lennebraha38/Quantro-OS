#ifndef TCP_H
#define TCP_H
#include "types.h"
void tcp_init(void);
int tcp_connect(uint32_t ip, uint16_t port);
int tcp_send(int sock, const void *data, uint32_t len);
int tcp_recv(int sock, void *buf, uint32_t len);
#endif
