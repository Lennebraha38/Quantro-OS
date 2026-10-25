#include "filemgr.h"
#include "uart.h"
#include "kernel.h"
#include "vfs.h"

void filemgr_init(void)
{
    uart_puts("[app:filemgr] Windows 11 File Explorer (Dosya Gezgini) hazir\r\n");
}

void filemgr_run(void)
{
    uart_puts("\r\n=======================================================\r\n");
    uart_puts(" QUANTRO OS - WINDOWS DOSYA GEZGiNi (EXPLORER.EXE)\r\n");
    uart_puts("=======================================================\r\n");
    uart_puts(" Suruculer:\r\n");
    uart_puts("   [ Yerel Disk (C:) ]  12.4 MB dolu / 1024 MB toplam (NTFS)\r\n");
    uart_puts("   [ EFI Bolumu (ESP) ] 4.2 MB dolu / 64 MB toplam (FAT32)\r\n\r\n");
    uart_puts(" Hizli Erisim Klasorleri:\r\n");
    uart_puts("   📁 Masaustu     (C:\\Users\\Quantro\\Desktop)\r\n");
    uart_puts("   📁 Belgeler     (C:\\Users\\Quantro\\Documents)\r\n");
    uart_puts("   📁 Indirilenler (C:\\Users\\Quantro\\Downloads)\r\n");
    uart_puts("   📁 Sistem       (C:\\Windows\\System32)\r\n\r\n");
    uart_puts(" C:\\ Disk Icerigi:\r\n");
    vfs_list();
    uart_puts("=======================================================\r\n");
}
