/*
 * Benim-OS Pro - Masaustu arayuz (Windows 11 esagini)
 *
 * Cizimler yalnizca framebuffer uzerine dogrudan yapilir. Klavye girisi
 * UART uzerinden alinir; fare surucusu bu surumde yoktur.
 */
#ifndef BENIMOS_GUI_H
#define BENIMOS_GUI_H

#include "types.h"

/* Tum ekrani cizer (baslangicta cagrilir) */
void gui_init(void);
void gui_redraw_all(void);

/* Saniyelik guncelleme: sadece saati ve sureyi yeniler */
void gui_tick(int64_t boot_time, uint64_t uptime_seconds);

/* Panic ekrani */
void gui_panic(const char *message);

#endif /* BENIMOS_GUI_H */
