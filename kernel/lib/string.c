/*
 * Benim-OS Pro - Freestanding bellek ve metin yardimcilari
 *
 * GCC/Clang -ffreestanding modunda donguleri memset/memcpy cagrilarina
 * cevirebildigi icin bu fonksiyonlar cekirdegin icinde tanimlidir.
 */
#include "kernel.h"

void *memset(void *dst, int value, size_t count)
{
    uint8_t *d = (uint8_t *)dst;
    uint8_t v = (uint8_t)value;

    while (count--)
        *d++ = v;

    return dst;
}

void *memcpy(void *dst, const void *src, size_t count)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    while (count--)
        *d++ = *s++;

    return dst;
}

void *memmove(void *dst, const void *src, size_t count)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    if (d == s || count == 0)
        return dst;

    if (d < s) {
        while (count--)
            *d++ = *s++;
    } else {
        d += count;
        s += count;
        while (count--)
            *--d = *--s;
    }

    return dst;
}

int memcmp(const void *a, const void *b, size_t count)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;

    while (count--) {
        if (*pa != *pb)
            return (int)*pa - (int)*pb;
        pa++;
        pb++;
    }

    return 0;
}

size_t strlen(const char *s)
{
    size_t len = 0;

    while (s[len])
        len++;

    return len;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }

    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n && *a && *a == *b) {
        a++;
        b++;
        n--;
    }

    if (n == 0)
        return 0;

    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

int str_starts_with(const char *s, const char *prefix)
{
    while (*prefix) {
        if (*s != *prefix)
            return 0;
        s++;
        prefix++;
    }

    return 1;
}
