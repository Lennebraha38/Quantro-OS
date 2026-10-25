#include "calculator.h"
#include "uart.h"
#include "kernel.h"

void calculator_init(void)
{
    uart_puts("[app:calculator] Windows 11 Modern Calculator Motoru hazir\r\n");
}

int calculator_calc(int a, char op, int b, int *res)
{
    if (!res) return -1;
    switch (op) {
    case '+': *res = a + b; return 0;
    case '-': *res = a - b; return 0;
    case '*': *res = a * b; return 0;
    case '/':
        if (b == 0) {
            uart_puts("[calc] Hata: Sifira bolme yapilamaz!\r\n");
            return -2;
        }
        *res = a / b;
        return 0;
    case '%':
        if (b == 0) return -2;
        *res = a % b;
        return 0;
    default:
        uart_printf("[calc] Desteklenmeyen islem: %c\r\n", op);
        return -1;
    }
}

void calculator_run(void)
{
    uart_puts("\r\n=========================================\r\n");
    uart_puts(" QUANTRO HESAP MAKINESI (Windows Calc)\r\n");
    uart_puts("=========================================\r\n");
    uart_puts(" Desteklenen islemler: + (topla), - (cikar), * (carp), / (bol), % (mod)\r\n");
    uart_puts(" Kullanim: 'calc <sayi1> <islem> <sayi2>'\r\n");
    uart_puts(" Ornek: calc 144 / 12  ->  12\r\n");
    uart_puts("=========================================\r\n");
}
