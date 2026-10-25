#include "browser.h"
#include "uart.h"
#include "kernel.h"
#include "net.h"

void browser_init(void)
{
    uart_puts("[app:browser] Microsoft Edge / Quantro Web Tarayicisi hazir\r\n");
}

void browser_navigate(const char *url)
{
    uart_printf("\r\n[Edge] '%s' adresine gidiliyor...\r\n", url);
    net_fetch(url);
}

void browser_run(void)
{
    uart_puts("\r\n=======================================================\r\n");
    uart_puts(" QUANTRO OS - MICROSOFT EDGE / WEB TARAYICI\r\n");
    uart_puts("=======================================================\r\n");
    uart_puts(" Surum: Edge Chromium 128.0 (Quantro AArch64 Edition)\r\n");
    uart_puts(" Yerlesik Koruma: SmartScreen ve Izleme Engelleme ETKIN\r\n\r\n");
    uart_puts(" Hizli Baglantilar:\r\n");
    uart_puts("   1. https://www.bing.com\r\n");
    uart_puts("   2. https://github.com/Lennebraha38/Quantro-OS\r\n");
    uart_puts("   3. https://learn.microsoft.com\r\n\r\n");
    uart_puts(" Komut satiri gezintisi icin: 'edge <url>' veya 'curl <url>'\r\n");
    uart_puts("=======================================================\r\n");
}
