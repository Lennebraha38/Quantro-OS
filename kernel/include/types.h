/*
 * Benim-OS Pro - Temel tip tanimlari
 *
 * Freestanding bir cekirdek oldugumuz icin libc basliklarina bagimli
 * degiliz. Tum tipler derleyici yerlesiklerinden turetilir; boylece
 * ayni kaynak hem aarch64-linux-gnu-gcc hem de clang ile derlenir.
 */
#ifndef BENIMOS_TYPES_H
#define BENIMOS_TYPES_H

typedef unsigned char      uint8_t;
typedef signed char        int8_t;
typedef unsigned short     uint16_t;
typedef signed short       int16_t;
typedef unsigned int       uint32_t;
typedef signed int         int32_t;
typedef unsigned long long uint64_t;
typedef signed long long   int64_t;

typedef __SIZE_TYPE__      size_t;
typedef __UINTPTR_TYPE__   uintptr_t;
typedef __INTPTR_TYPE__    intptr_t;
typedef __builtin_va_list  va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap)         __builtin_va_end(ap)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_copy(dst, src)  __builtin_va_copy(dst, src)

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef int bool;
#define true  1
#define false 0

#define UINT64_MAX 0xFFFFFFFFFFFFFFFFULL
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#endif /* BENIMOS_TYPES_H */
