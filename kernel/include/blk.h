#ifndef BLK_H
#define BLK_H
#include "types.h"
void blk_init(void);
int blk_read_sector(uint64_t lba, void *buf);
int blk_write_sector(uint64_t lba, const void *buf);
#endif
