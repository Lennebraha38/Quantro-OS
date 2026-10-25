/*
 * Quantro OS - Cekirdek genel arayuzu
 * Windows 11 Mimari Uyumluluk Surumu
 */
#ifndef QUANTRO_KERNEL_H
#define QUANTRO_KERNEL_H

#include "types.h"

#define KERNEL_NAME    "Quantro OS"
#define KERNEL_VERSION "0.4.0-wincomp"
#define KERNEL_ARCH    "AArch64"

/* QEMU virt makinesindeki PL011 UART'in fiziksel adresi */
#define PL011_PHYS_BASE 0x09000000ULL

/* lib/string.c */
void  *memset(void *dst, int value, size_t count);
void  *memcpy(void *dst, const void *src, size_t count);
void  *memmove(void *dst, const void *src, size_t count);
int    memcmp(const void *a, const void *b, size_t count);
size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
int    strcasecmp(const char *a, const char *b);
int    str_starts_with(const char *s, const char *prefix);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strchr(const char *s, int c);
int    atoi(const char *s);

/* lib/format.c */
int  kvsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int  ksnprintf(char *buf, size_t size, const char *fmt, ...);
void u64_to_dec(uint64_t value, char *buf);
void u64_to_hex(uint64_t value, char *buf);

/* Cekirdek durdurma yardimcilari */
void kernel_panic(const char *message) __attribute__((noreturn));
void halt_forever(void) __attribute__((noreturn));

/* Zaman yardimcilari (arch/aarch64) */
uint64_t timer_frequency(void);
uint64_t timer_ticks(void);
void     timer_init(void);

#endif /* QUANTRO_KERNEL_H */
