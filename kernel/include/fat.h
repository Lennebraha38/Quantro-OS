#ifndef FAT_H
#define FAT_H
#include "types.h"
void fat_init(void);
int fat_mount(void);
int fat_open(const char *path);
int fat_read(int fd, void *buf, uint32_t len);
#endif
