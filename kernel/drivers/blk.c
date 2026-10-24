#include "blk.h"
#include "virtio.h"
#include "uart.h"
void blk_init(void){
    virtio_mmio_init();
    uart_puts("[drv:blk] virtio-blk bound\r\n");
}
int blk_read_sectors(uint64_t lba, uint32_t count, void *buf){ return virtio_blk_read_sectors(lba,count,buf); }
int blk_write_sectors(uint64_t lba, uint32_t count, const void *buf){ return virtio_blk_write_sectors(lba,count,buf); }
