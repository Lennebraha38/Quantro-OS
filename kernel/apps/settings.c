#include "settings.h"
#include "uart.h"
#include "kernel.h"
#include "sound.h"

void settings_init(void)
{
    uart_puts("[app:settings] Windows 11 Fluent Settings Hub hazir\r\n");
}

void settings_run(void)
{
    uart_puts("\r\n=======================================================\r\n");
    uart_puts(" QUANTRO OS - WINDOWS 11 AYARLAR MERKEZi\r\n");
    uart_puts("=======================================================\r\n");
    uart_puts(" 1. Sistem ve Cihaz:\r\n");
    uart_puts("    - Cihaz Adi: QUANTRO-PC (ARM64 Virtual Machine)\r\n");
    uart_puts("    - Islemci: ARM Cortex-A53 @ 1.50 GHz (4 Cores)\r\n");
    uart_puts("    - Yuklu RAM: 1024 MB (1.00 GB)\r\n");
    uart_puts("    - Sistem Turu: 64-bit isletim sistemi, ARM64 tabanli islemci\r\n\r\n");
    uart_puts(" 2. Goruntu ve Ekran:\r\n");
    uart_puts("    - Cozunurluk: 1280 x 800 piksel\r\n");
    uart_puts("    - Yenileme Hizi: 60 Hz\r\n");
    uart_puts("    - Renk Formati: 32-bit ARGB8888 (True Color)\r\n\r\n");
    uart_puts(" 3. Kisisellestirme:\r\n");
    uart_puts("    - Tema: Windows 11 Dark Mode (Mica & Acrylic Efektleri)\r\n");
    uart_puts("    - Vurgu Rengi: Windows 11 Cyan / Sky Blue (#4CC2FF)\r\n\r\n");
    uart_printf(" 4. Ses ve Cikis:\r\n");
    uart_printf("    - Varsayilan Aygit: VirtIO High Definition Audio\r\n");
    uart_printf("    - Ses Seviyesi: %%%d\r\n\r\n", sound_get_volume());
    uart_puts(" 5. Windows Update:\r\n");
    uart_puts("    - Durum: Guncelsiniz (Son denetim: Bugun)\r\n");
    uart_puts("    - Quantro OS 24H2 Feature Update kurulu.\r\n");
    uart_puts("=======================================================\r\n");
}
