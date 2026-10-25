/*
 * Quantro OS - Gelismis Framebuffer ve Grafik Cizim Motoru
 * Donanimsal Hizlandirma, Double Buffering, Blur ve Antialiasing
 */
#ifndef QUANTRO_FB_H
#define QUANTRO_FB_H

#include "types.h"

struct limine_framebuffer;

struct fb_surface {
    volatile uint32_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t pitch_pixels;
    uint16_t bpp;
    uint8_t  red_size,   red_shift;
    uint8_t  green_size, green_shift;
    uint8_t  blue_size,  blue_shift;
    int      ready;
};

extern struct fb_surface g_fb;

void     fb_init(struct limine_framebuffer *lfb);
int      fb_ready(void);
uint32_t fb_width(void);
uint32_t fb_height(void);
uint32_t fb_get_pixel(int x, int y);
void     fb_put_pixel(int x, int y, uint32_t rgb);

/* Geometrik Cizim */
void fb_fill_rect(int x, int y, int w, int h, uint32_t rgb);
void fb_fill_round_rect(int x, int y, int w, int h, int radius, uint32_t rgb);
void fb_blend_rect(int x, int y, int w, int h, uint32_t rgb, uint8_t alpha);
void fb_blend_round_rect(int x, int y, int w, int h, int radius, uint32_t rgb, uint8_t alpha);
void fb_draw_border(int x, int y, int w, int h, int thickness, uint32_t rgb);
void fb_draw_round_border(int x, int y, int w, int h, int radius, int thickness, uint32_t rgb);

/* Windows 11 Acrylic Blur / Buzlu Cam Efekti */
void fb_blur_rect(int x, int y, int w, int h, int radius);
void fb_draw_acrylic_panel(int x, int y, int w, int h, int radius, uint32_t tint_rgb, uint8_t opacity);

/* Antialiased & Modern Metin */
void fb_draw_char(int x, int y, char c, uint32_t rgb, int scale);
void fb_draw_text(int x, int y, const char *text, uint32_t rgb, int scale);
void fb_draw_text_centered(int cx, int y, const char *text, uint32_t rgb, int scale);
uint32_t fb_text_width(const char *text, int scale);

#endif /* QUANTRO_FB_H */
