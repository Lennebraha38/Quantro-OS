#ifndef QUANTRO_NET_H
#define QUANTRO_NET_H

#include "types.h"

typedef struct {
    uint8_t  ip[4];
    uint8_t  netmask[4];
    uint8_t  gateway[4];
    uint8_t  dns[4];
    uint8_t  mac[6];
    int      link_up;
    uint32_t rx_packets;
    uint32_t tx_packets;
} net_config_t;

void net_init(void);
int  net_send(const void *pkt, uint32_t len);
int  net_recv(void *pkt, uint32_t *len);
void net_ipconfig(void);
void net_ping(const char *ip_str);
void net_netstat(void);
void net_fetch(const char *url);
void arp_resolve(uint32_t ip);
void icmp_echo(uint32_t ip);

#endif /* QUANTRO_NET_H */
