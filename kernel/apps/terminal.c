#include "terminal.h"
#include "uart.h"
#include "kernel.h"

void terminal_init(void)
{
    uart_puts("[app:terminal] Windows Terminal / PowerShell 7.4 hazir\r\n");
}

void terminal_run(void)
{
    uart_puts("\r\nWindows PowerShell\r\n");
    uart_puts("Copyright (C) Microsoft Corporation / Quantro Project. Tum haklari saklidir.\r\n\r\n");
    uart_puts("Yeni platformlar arasi PowerShell'i yukleyin: https://aka.ms/pscore6\r\n\r\n");
    uart_puts("PS C:\\Users\\Quantro> \r\n");
}
