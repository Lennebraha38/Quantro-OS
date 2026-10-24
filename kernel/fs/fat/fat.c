#include "fat.h"
#include "uart.h"
/* FAT32 iskeleti: BPB okuma + cluster zinciri + dosya acma */
void fat_init(void){ uart_puts("[fs:fat] FAT32 init (stub)\r\n"); }
int fat_mount(void){ uart_puts("[fs:fat] mount (stub)\r\n"); return 0; }
int fat_open(const char *path){ (void)path; uart_puts("[fs:fat] open (stub)\r\n"); return 0; }
int fat_read(int fd, void *buf, uint32_t len){ (void)fd;(void)buf;(void)len; return 0; /* stub */ }
