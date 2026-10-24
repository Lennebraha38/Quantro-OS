#include "virtio.h"
#include "uart.h"
#include "types.h"

/* QEMU virt machine: virtio-mmio devices at 0x0a000000, stride 0x200 */
#define VIRTIO_MMIO_BASE 0x0a000000ULL
#define VIRTIO_MMIO_STRIDE 0x200
#define VIRTIO_MMIO_MAX 32

/* MMIO register offsets */
#define MMIO_MAGIC 0x000
#define MMIO_VERSION 0x004
#define MMIO_DEVICE_ID 0x008
#define MMIO_VENDOR_ID 0x00c
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
#define MMIO_QUEUE_DESC_LOW 0x080
#define MMIO_QUEUE_DESC_HIGH 0x084
#define MMIO_QUEUE_AVAIL_LOW 0x090
#define MMIO_QUEUE_AVAIL_HIGH 0x094
#define MMIO_QUEUE_USED_LOW 0x0a0
#define MMIO_QUEUE_USED_HIGH 0x0a4

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

static volatile uint32_t *mmio(uint64_t base, uint32_t off){
    return (volatile uint32_t *)(base + off);
}

static uint64_t blk_base = 0;
static uint64_t blk_capacity = 0;

void virtio_mmio_init(void){
    for(int i=0;i<VIRTIO_MMIO_MAX;i++){
        uint64_t base = VIRTIO_MMIO_BASE + (uint64_t)i*VIRTIO_MMIO_STRIDE;
        uint32_t magic = *mmio(base, MMIO_MAGIC);
        if(magic != 0x74726976) continue;
        uint32_t dev = *mmio(base, MMIO_DEVICE_ID);
        uart_printf("[virtio] mmio slot %d dev=%u\r\n", i, dev);
        if(dev == 2){ /* block device */
            blk_base = base;
            *mmio(base, MMIO_STATUS) = 0;
            *mmio(base, MMIO_STATUS) = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
            /* features */
            *mmio(base, MMIO_HOST_FEATURES_SEL) = 0;
            (void)*mmio(base, MMIO_HOST_FEATURES);
            *mmio(base, MMIO_GUEST_FEATURES_SEL) = 0;
            *mmio(base, MMIO_GUEST_FEATURES) = 0;
            *mmio(base, MMIO_STATUS) = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK;
            *mmio(base, MMIO_STATUS) = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK;
            uart_puts("[virtio-blk] initialized (basic)\r\n");
            return;
        }
    }
    uart_puts("[virtio] no blk device found\r\n");
}

/* Minimal sector R/W: for now just report not available cleanly */
int virtio_blk_read_sectors(uint64_t lba, uint32_t count, void *buf){
    (void)lba;(void)count;(void)buf;
    if(!blk_base) return -1;
    return 0;
}
int virtio_blk_write_sectors(uint64_t lba, uint32_t count, const void *buf){
    (void)lba;(void)count;(void)buf;
    if(!blk_base) return -1;
    return 0;
}
uint64_t virtio_blk_capacity(void){ return blk_capacity; }
