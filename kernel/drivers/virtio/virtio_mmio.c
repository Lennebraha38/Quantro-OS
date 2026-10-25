#include "virtio.h"
#include "uart.h"
#include "types.h"

#define VIRTIO_MMIO_BASE 0x0a000000ULL
#define VIRTIO_MMIO_STRIDE 0x200
#define VIRTIO_MMIO_MAX 32

#define MMIO_MAGIC 0x000
#define MMIO_VERSION 0x004
#define MMIO_DEVICE_ID 0x008
#define MMIO_HOST_FEATURES 0x010
#define MMIO_HOST_FEATURES_SEL 0x014
#define MMIO_GUEST_FEATURES 0x020
#define MMIO_GUEST_FEATURES_SEL 0x024
#define MMIO_GUEST_PAGE_SIZE 0x028
#define MMIO_QUEUE_SEL 0x030
#define MMIO_QUEUE_NUM_MAX 0x034
#define MMIO_QUEUE_NUM 0x038
#define MMIO_QUEUE_ALIGN 0x03c
#define MMIO_QUEUE_PFN 0x040
#define MMIO_QUEUE_READY 0x044
#define MMIO_QUEUE_NOTIFY 0x050
#define MMIO_INTERRUPT_STATUS 0x060
#define MMIO_INTERRUPT_ACK 0x064
#define MMIO_STATUS 0x070
#define MMIO_CONFIG 0x100

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

/* virtqueue size */
#define QSZ 16

typedef struct { uint64_t addr; uint32_t len; uint16_t flags; uint16_t next; } vring_desc_t;
typedef struct { uint16_t flags; uint16_t idx; uint16_t ring[QSZ]; } vring_avail_t;
typedef struct { uint32_t id; uint32_t len; } vring_used_elem_t;
typedef struct { uint16_t flags; uint16_t idx; vring_used_elem_t ring[QSZ]; } vring_used_t;

typedef struct { uint32_t type; uint32_t reserved; uint64_t sector; } blk_req_hdr_t;

static volatile uint32_t *mmio(uint64_t base, uint32_t off){ return (volatile uint32_t *)(base + off); }
static void mmio_w(uint64_t base,uint32_t off,uint32_t v){ *mmio(base,off)=v; }
static uint32_t mmio_r(uint64_t base,uint32_t off){ return *mmio(base,off); }

static uint64_t blk_base = 0;
static uint64_t blk_capacity = 0;

/* virtqueue storage: tek contiguous buffer (legacy virtio-mmio gereksinimi) */
typedef struct {
    vring_desc_t descs[QSZ];
    vring_avail_t avail;
    uint8_t pad[4096];
    vring_used_t used;
} vring_t;
static vring_t vring __attribute__((aligned(4096)));

static blk_req_hdr_t req_hdr __attribute__((aligned(16)));
static uint8_t status_byte __attribute__((aligned(16)));

static int setup_queue(uint64_t base){
    mmio_w(base, MMIO_QUEUE_SEL, 0);
    uint32_t max = mmio_r(base, MMIO_QUEUE_NUM_MAX);
    if(max == 0) return -1;
    uint32_t n = (max < QSZ) ? max : QSZ;
    mmio_w(base, MMIO_QUEUE_NUM, n);
    mmio_w(base, MMIO_QUEUE_ALIGN, 4096);
    mmio_w(base, MMIO_GUEST_PAGE_SIZE, 4096);
    mmio_w(base, MMIO_QUEUE_PFN, (uint32_t)(((uint64_t)&vring) >> 12));
    /* Modern queue ready */
    mmio_w(base, MMIO_QUEUE_READY, 1);
    /* reset indices */
    vring.avail.idx = 0; vring.avail.flags = 0;
    vring.used.idx = 0; vring.used.flags = 0;
    return 0;
}

void virtio_mmio_init(void){
    for(int i=0;i<VIRTIO_MMIO_MAX;i++){
        uint64_t base = VIRTIO_MMIO_BASE + (uint64_t)i*VIRTIO_MMIO_STRIDE;
        uint32_t magic = mmio_r(base, MMIO_MAGIC);
        if(magic != 0x74726976) continue;
        uint32_t dev = mmio_r(base, MMIO_DEVICE_ID);
        uart_printf("[virtio] mmio slot %d dev=%u\r\n", i, dev);
        if(dev == 2){
            blk_base = base;
            mmio_w(base, MMIO_STATUS, 0);
            mmio_w(base, MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);
            mmio_w(base, MMIO_HOST_FEATURES_SEL, 0);
            (void)mmio_r(base, MMIO_HOST_FEATURES);
            mmio_w(base, MMIO_GUEST_FEATURES_SEL, 0);
            mmio_w(base, MMIO_GUEST_FEATURES, 0);
            mmio_w(base, MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);
            if(setup_queue(base)!=0){ uart_puts("[virtio-blk] queue setup failed\r\n"); return; }
            mmio_w(base, MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);
            /* capacity from config (in 512-byte sectors, little-endian 64-bit at config+0) */
            uint32_t lo = mmio_r(base, MMIO_CONFIG);
            uint32_t hi = mmio_r(base, MMIO_CONFIG+4);
            blk_capacity = ((uint64_t)hi << 32) | lo;
            uart_printf("[virtio-blk] initialized capacity=%llu sectors\r\n", (unsigned long long)blk_capacity);
            return;
        }
    }
    uart_puts("[virtio] no blk device found\r\n");
}

static int do_request(uint32_t type, uint64_t lba, uint32_t count, void *buf){
    if(!blk_base) return -1;
    req_hdr.type = type;
    req_hdr.reserved = 0;
    req_hdr.sector = lba;
    status_byte = 0xFF;

    /* descriptor 0: header (device reads) */
    vring.descs[0].addr = (uint64_t)&req_hdr;
    vring.descs[0].len = sizeof(req_hdr);
    vring.descs[0].flags = 1; /* NEXT */
    vring.descs[0].next = 1;
    /* descriptor 1: data */
    vring.descs[1].addr = (uint64_t)buf;
    vring.descs[1].len = count * 512;
    vring.descs[1].flags = (type==VIRTIO_BLK_T_IN ? 2 : 0) | 1; /* WRITE? device writes for IN -> VIRTQ_DESC_F_WRITE=2; NEXT */
    vring.descs[1].next = 2;
    /* descriptor 2: status (device writes) */
    vring.descs[2].addr = (uint64_t)&status_byte;
    vring.descs[2].len = 1;
    vring.descs[2].flags = 2; /* WRITE */
    vring.descs[2].next = 0;

    uint16_t head = 0;
    vring.avail.ring[vring.avail.idx % QSZ] = head;
    __sync_synchronize();
    vring.avail.idx++;

    mmio_w(blk_base, MMIO_QUEUE_NOTIFY, 0);

    /* poll used */
    uint16_t last = vring.used.idx;
    for(volatile int t=0;t<1000000;t++){
        if(vring.used.idx != last) break;
    }
    if(vring.used.idx == last) return -2; /* timeout */
    if(status_byte != 0) return -3;
    return 0;
}

int virtio_blk_read_sectors(uint64_t lba, uint32_t count, void *buf){
    return do_request(VIRTIO_BLK_T_IN, lba, count, buf);
}
int virtio_blk_write_sectors(uint64_t lba, uint32_t count, const void *buf){
    return do_request(VIRTIO_BLK_T_OUT, lba, count, (void*)buf);
}
uint64_t virtio_blk_capacity(void){ return blk_capacity; }
