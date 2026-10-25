#ifndef QUANTRO_NET_H
#define QUANTRO_NET_H

#include "types.h"

#pragma pack(push, 1)
typedef struct {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6];
    uint16_t ethertype; /* 0x0800: IPv4, 0x0806: ARP */
} eth_hdr_t;

typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_len;
    uint8_t  proto_len;
    uint16_t opcode;    /* 1: Request, 2: Reply */
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
} arp_pkt_t;

typedef struct {
    uint8_t  ver_ihl;   /* 0x45 */
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  protocol;  /* 1: ICMP, 6: TCP, 17: UDP */
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} ipv4_hdr_t;

typedef struct {
    uint8_t  type;      /* 8: Request, 0: Reply */
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} icmp_hdr_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} udp_hdr_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  data_offset;
    uint8_t  flags;     /* SYN, ACK, FIN, RST, PSH */
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
} tcp_hdr_t;
#pragma pack(pop)

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

void     net_init(void);
int      net_send_packet(const void *packet, uint32_t len);
void     net_receive_packet(const void *packet, uint32_t len);
uint16_t net_checksum(const void *data, size_t len);
void     net_send_arp_request(uint32_t target_ip);
void     net_send_icmp_echo(uint32_t dest_ip, uint16_t seq);
void     net_ipconfig(void);
void     net_ping(const char *ip_str);
void     net_netstat(void);
void     net_fetch(const char *url);

#endif /* QUANTRO_NET_H */
