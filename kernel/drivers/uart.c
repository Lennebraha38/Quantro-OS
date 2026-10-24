/*
 * Benim-OS Pro - PL011 UART surucusu
 *
 * QEMU virt makinesinde PL011 fiziksel 0x09000000 adresindedir.
 * MMU acik oldugu icin surucu, Limine'in HHDM ofseti eklenmis
 * sanal adresi ile calisir.
 */
#include "uart.h"
#include "kernel.h"

#define UART_DR    0x00
#define UART_FR    0x18
#define UART_IBRD  0x24
#define UART_FBRD  0x28
#define UART_LCRH  0x2C
#define UART_CR    0x30
#define UART_ICR   0x44

#define FR_RXFE    (1u << 4)
#define FR_TXFF    (1u << 5)

#define CR_UARTEN  (1u << 0)
#define CR_TXE     (1u << 8)
#define CR_RXE     (1u << 9)

#define LCRH_FEN   (1u << 4)
#define LCRH_WLEN8 (3u << 5)

static volatile uint32_t *uart_regs;

static inline void uart_write_reg(uint32_t offset, uint32_t value)
{
    uart_regs[offset / 4] = value;
}

static inline uint32_t uart_read_reg(uint32_t offset)
{
    return uart_regs[offset / 4];
}

void uart_init(uintptr_t base)
{
    uart_regs = (volatile uint32_t *)base;

    uart_write_reg(UART_CR, 0);
    uart_write_reg(UART_ICR, 0x7FF);

    /* 24 MHz referans saatinde 115200 baud (QEMU degeri yok sayar) */
    uart_write_reg(UART_IBRD, 13);
    uart_write_reg(UART_FBRD, 37);

    uart_write_reg(UART_LCRH, LCRH_FEN | LCRH_WLEN8);
    uart_write_reg(UART_CR, CR_UARTEN | CR_TXE | CR_RXE);
}

void uart_putc(char c)
{
    if (!uart_regs)
        return;

    if (c == '\n')
        uart_putc('\r');

    while (uart_read_reg(UART_FR) & FR_TXFF)
        ;

    uart_write_reg(UART_DR, (uint32_t)(uint8_t)c);
}

void uart_puts(const char *s)
{
    if (!s)
        return;

    while (*s)
        uart_putc(*s++);
}

void uart_printf(const char *fmt, ...)
{
    char buffer[512];
    va_list ap;

    va_start(ap, fmt);
    kvsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    uart_puts(buffer);
}

int uart_rx_ready(void)
{
    if (!uart_regs)
        return 0;

    return (uart_read_reg(UART_FR) & FR_RXFE) == 0;
}

char uart_getc(void)
{
    while (!uart_rx_ready())
        ;

    return (char)(uart_read_reg(UART_DR) & 0xFF);
}
