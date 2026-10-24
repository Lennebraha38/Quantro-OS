#include "blk.h"
#include "uart.h"
/* virtio-blk iskeleti: gercek I/O icin virtio kuyruk + MMIO gerekir */
static int blk_ready = 0;
void blk_init(void){ blk_ready=1; uart_puts("[drv:blk] virtio-blk init (stub, ready=1)\r\n"); }
int blk_read_sector(uint64_t lba, void *buf){ (void)lba;(void)buf; if(!blk_ready) return -1; return 0; /* stub */ }
int blk_write_sector(uint64_t lba, const void *buf){ (void)lba;(void)buf; if(!blk_ready) return -1; return 0; /* stub */ }
