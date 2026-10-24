#ifndef VIRTIO_H
#define VIRTIO_H
#include "types.h"
void virtio_mmio_init(void);
int virtio_blk_read_sectors(uint64_t lba, uint32_t count, void *buf);
int virtio_blk_write_sectors(uint64_t lba, uint32_t count, const void *buf);
uint64_t virtio_blk_capacity(void);
#endif
