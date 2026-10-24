/*
 * Benim-OS Pro - Minimal printf ailesi
 *
 * Desteklenen donusumler: %s %c %u %d %x %X %p %% ve 0 ile
 * doldurulmus genislik belirtecleri (or. %08x). Tum tam sayilar
 * 64-bit olarak islenir; 'l' / 'll' on ekleri yok sayilir.
 */
#include "kernel.h"

struct out_buf {
    char  *data;
    size_t size;
    size_t pos;
};

static void out_char(struct out_buf *ob, char c)
{
    if (ob->size && ob->pos + 1 < ob->size)
        ob->data[ob->pos] = c;
    ob->pos++;
}

static void out_str(struct out_buf *ob, const char *s)
{
    if (!s)
        s = "(null)";
    while (*s)
        out_char(ob, *s++);
}

static void out_uint(struct out_buf *ob, uint64_t value, uint32_t base,
                     int uppercase, int width, int zero_pad)
{
    char tmp[32];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int len = 0;

    if (value == 0) {
        tmp[len++] = '0';
    } else {
        while (value) {
            tmp[len++] = digits[value % base];
            value /= base;
        }
    }

    for (int i = len; i < width; i++)
        out_char(ob, zero_pad ? '0' : ' ');

    while (len--)
        out_char(ob, tmp[len]);
}

int kvsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    struct out_buf ob = { buf, size, 0 };

    while (*fmt) {
        if (*fmt != '%') {
            out_char(&ob, *fmt++);
            continue;
        }

        fmt++;

        int zero_pad = 0;
        int width = 0;

        if (*fmt == '0') {
            zero_pad = 1;
            fmt++;
        }

        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        if (*fmt == 'l') {
            fmt++;
            if (*fmt == 'l')
                fmt++;
        }

        switch (*fmt) {
        case 's':
            out_str(&ob, va_arg(ap, const char *));
            break;
        case 'c':
            out_char(&ob, (char)va_arg(ap, int));
            break;
        case 'u':
            out_uint(&ob, va_arg(ap, uint64_t), 10, 0, width, zero_pad);
            break;
        case 'd': {
            int64_t v = va_arg(ap, int64_t);
            if (v < 0) {
                out_char(&ob, '-');
                out_uint(&ob, (uint64_t)(-v), 10, 0, width, zero_pad);
            } else {
                out_uint(&ob, (uint64_t)v, 10, 0, width, zero_pad);
            }
            break;
        }
        case 'x':
            out_uint(&ob, va_arg(ap, uint64_t), 16, 0, width, zero_pad);
            break;
        case 'X':
            out_uint(&ob, va_arg(ap, uint64_t), 16, 1, width, zero_pad);
            break;
        case 'p':
            out_str(&ob, "0x");
            out_uint(&ob, (uint64_t)(uintptr_t)va_arg(ap, void *), 16, 0,
                     width, 1);
            break;
        case '%':
            out_char(&ob, '%');
            break;
        case '\0':
            fmt--;
            break;
        default:
            out_char(&ob, '%');
            out_char(&ob, *fmt);
            break;
        }

        fmt++;
    }

    if (ob.size)
        ob.data[ob.pos < ob.size ? ob.pos : ob.size - 1] = '\0';

    return (int)ob.pos;
}

int ksnprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    int result;

    va_start(ap, fmt);
    result = kvsnprintf(buf, size, fmt, ap);
    va_end(ap);

    return result;
}

void u64_to_dec(uint64_t value, char *buf)
{
    char tmp[24];
    int len = 0;

    if (value == 0)
        tmp[len++] = '0';
    else {
        while (value) {
            tmp[len++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    while (len)
        *buf++ = tmp[--len];

    *buf = '\0';
}

void u64_to_hex(uint64_t value, char *buf)
{
    static const char digits[] = "0123456789abcdef";
    char tmp[20];
    int len = 0;

    if (value == 0)
        tmp[len++] = '0';
    else {
        while (value) {
            tmp[len++] = digits[value & 0xF];
            value >>= 4;
        }
    }

    while (len)
        *buf++ = tmp[--len];

    *buf = '\0';
}
