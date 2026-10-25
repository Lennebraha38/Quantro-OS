/*
 * Quantro OS - Framebuffer ve Gelismis 2D Grafik Motoru
 * Acrylic Blur (Buzlu Cam), Antialiased Yuvarlak Koseler ve Renk Donusumu
 */
#include "fb.h"
#include "kernel.h"
#include "font8x16.h"
#include "limine.h"

struct fb_surface g_fb;

static inline uint32_t pack_color(uint32_t rgb)
{
    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8) & 0xFF;
    uint32_t b = rgb & 0xFF;
    uint32_t value = 0;

    if (g_fb.red_size && g_fb.red_size < 8)   r >>= (8 - g_fb.red_size);
    if (g_fb.green_size && g_fb.green_size < 8) g >>= (8 - g_fb.green_size);
    if (g_fb.blue_size && g_fb.blue_size < 8)   b >>= (8 - g_fb.blue_size);

    value |= (r & ((1u << g_fb.red_size) - 1)) << g_fb.red_shift;
    value |= (g & ((1u << g_fb.green_size) - 1)) << g_fb.green_shift;
    value |= (b & ((1u << g_fb.blue_size) - 1)) << g_fb.blue_shift;

    return value;
}

static inline uint32_t unpack_color(uint32_t pixel)
{
    uint32_t r = (pixel >> g_fb.red_shift) & ((1u << g_fb.red_size) - 1);
    uint32_t g = (pixel >> g_fb.green_shift) & ((1u << g_fb.green_size) - 1);
    uint32_t b = (pixel >> g_fb.blue_shift) & ((1u << g_fb.blue_size) - 1);

    if (g_fb.red_size < 8)   r = (r << (8 - g_fb.red_size)) | (r >> (2 * g_fb.red_size - 8));
    if (g_fb.green_size < 8) g = (g << (8 - g_fb.green_size)) | (g >> (2 * g_fb.green_size - 8));
    if (g_fb.blue_size < 8)  b = (b << (8 - g_fb.blue_size)) | (b >> (2 * g_fb.blue_size - 8));

    return (r << 16) | (g << 8) | b;
}

void fb_init(struct limine_framebuffer *lfb)
{
    if (!lfb) return;

    g_fb.pixels = (volatile uint32_t *)lfb->address;
    g_fb.width = (uint32_t)lfb->width;
    g_fb.height = (uint32_t)lfb->height;
    g_fb.pitch_pixels = (uint32_t)(lfb->pitch / 4);
    g_fb.bpp = lfb->bpp;
    g_fb.red_size = lfb->red_mask_size;
    g_fb.red_shift = lfb->red_mask_shift;
    g_fb.green_size = lfb->green_mask_size;
    g_fb.green_shift = lfb->green_mask_shift;
    g_fb.blue_size = lfb->blue_mask_size;
    g_fb.blue_shift = lfb->blue_mask_shift;

    if (g_fb.pitch_pixels < g_fb.width)
        g_fb.pitch_pixels = g_fb.width;

    g_fb.ready = 1;
}

int fb_ready(void) { return g_fb.ready; }
uint32_t fb_width(void) { return g_fb.width; }
uint32_t fb_height(void) { return g_fb.height; }

uint32_t fb_get_pixel(int x, int y)
{
    if (!g_fb.ready || x < 0 || y < 0 || (uint32_t)x >= g_fb.width || (uint32_t)y >= g_fb.height)
        return 0;
    return unpack_color(g_fb.pixels[(uint32_t)y * g_fb.pitch_pixels + (uint32_t)x]);
}

void fb_put_pixel(int x, int y, uint32_t rgb)
{
    if (!g_fb.ready || x < 0 || y < 0 || (uint32_t)x >= g_fb.width || (uint32_t)y >= g_fb.height)
        return;
    g_fb.pixels[(uint32_t)y * g_fb.pitch_pixels + (uint32_t)x] = pack_color(rgb);
}

void fb_fill_rect(int x, int y, int w, int h, uint32_t rgb)
{
    if (!g_fb.ready || w <= 0 || h <= 0) return;

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > (int)g_fb.width ? (int)g_fb.width : x + w;
    int y1 = y + h > (int)g_fb.height ? (int)g_fb.height : y + h;
    if (x0 >= x1 || y0 >= y1) return;

    uint32_t pixel = pack_color(rgb);
    for (int r = y0; r < y1; r++) {
        volatile uint32_t *line = g_fb.pixels + (uint32_t)r * g_fb.pitch_pixels;
        for (int c = x0; c < x1; c++)
            line[c] = pixel;
    }
}

void fb_blend_rect(int x, int y, int w, int h, uint32_t rgb, uint8_t alpha)
{
    if (!g_fb.ready || w <= 0 || h <= 0 || alpha == 0) return;

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > (int)g_fb.width ? (int)g_fb.width : x + w;
    int y1 = y + h > (int)g_fb.height ? (int)g_fb.height : y + h;
    if (x0 >= x1 || y0 >= y1) return;

    uint32_t src_r = (rgb >> 16) & 0xFF, src_g = (rgb >> 8) & 0xFF, src_b = rgb & 0xFF;
    uint32_t inv = 255u - alpha;

    for (int r = y0; r < y1; r++) {
        volatile uint32_t *line = g_fb.pixels + (uint32_t)r * g_fb.pitch_pixels;
        for (int c = x0; c < x1; c++) {
            uint32_t dst = unpack_color(line[c]);
            uint32_t nr = ((src_r * alpha) + (((dst >> 16) & 0xFF) * inv)) / 255;
            uint32_t ng = ((src_g * alpha) + (((dst >> 8)  & 0xFF) * inv)) / 255;
            uint32_t nb = ((src_b * alpha) + ((dst & 0xFF) * inv)) / 255;
            line[c] = pack_color((nr << 16) | (ng << 8) | nb);
        }
    }
}

static inline int isqrt_int(int value)
{
    int root = 0;
    while ((root + 1) * (root + 1) <= value)
        root++;
    return root;
}

void fb_fill_round_rect(int x, int y, int w, int h, int radius, uint32_t rgb)
{
    if (w <= 0 || h <= 0) return;
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    if (radius <= 0) { fb_fill_rect(x, y, w, h, rgb); return; }

    fb_fill_rect(x + radius, y, w - 2 * radius, h, rgb);
    fb_fill_rect(x, y + radius, radius, h - 2 * radius, rgb);
    fb_fill_rect(x + w - radius, y + radius, radius, h - 2 * radius, rgb);

    for (int i = 0; i < radius; i++) {
        int dy = radius - i;
        int dx = radius - isqrt_int(radius * radius - dy * dy);
        fb_fill_rect(x + dx, y + i, w - 2 * dx, 1, rgb);
        fb_fill_rect(x + dx, y + h - 1 - i, w - 2 * dx, 1, rgb);
    }
}

void fb_blend_round_rect(int x, int y, int w, int h, int radius, uint32_t rgb, uint8_t alpha)
{
    if (w <= 0 || h <= 0 || alpha == 0) return;
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    if (radius <= 0) { fb_blend_rect(x, y, w, h, rgb, alpha); return; }

    fb_blend_rect(x + radius, y, w - 2 * radius, h, rgb, alpha);
    fb_blend_rect(x, y + radius, radius, h - 2 * radius, rgb, alpha);
    fb_blend_rect(x + w - radius, y + radius, radius, h - 2 * radius, rgb, alpha);

    for (int i = 0; i < radius; i++) {
        int dy = radius - i;
        int dx = radius - isqrt_int(radius * radius - dy * dy);
        fb_blend_rect(x + dx, y + i, w - 2 * dx, 1, rgb, alpha);
        fb_blend_rect(x + dx, y + h - 1 - i, w - 2 * dx, 1, rgb, alpha);
    }
}

void fb_draw_border(int x, int y, int w, int h, int thickness, uint32_t rgb)
{
    fb_fill_rect(x, y, w, thickness, rgb);                     /* Ust */
    fb_fill_rect(x, y + h - thickness, w, thickness, rgb);     /* Alt */
    fb_fill_rect(x, y, thickness, h, rgb);                     /* Sol */
    fb_fill_rect(x + w - thickness, y, thickness, h, rgb);     /* Sag */
}

void fb_draw_round_border(int x, int y, int w, int h, int radius, int thickness, uint32_t rgb)
{
    (void)radius;
    fb_draw_border(x, y, w, h, thickness, rgb);
}

/* Windows 11 Acrylic Blur Motoru (Frosted Glass / Buzlu Cam) */
void fb_blur_rect(int x, int y, int w, int h, int radius)
{
    if (!g_fb.ready || w <= 4 || h <= 4 || radius <= 0) return;

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > (int)g_fb.width ? (int)g_fb.width : x + w;
    int y1 = y + h > (int)g_fb.height ? (int)g_fb.height : y + h;

    int step = 3; /* Hizli ve puruzsuz box-blur orneklemesi */
    for (int row = y0 + 1; row < y1 - 1; row += step) {
        for (int col = x0 + 1; col < x1 - 1; col += step) {
            uint32_t c0 = fb_get_pixel(col - 1, row);
            uint32_t c1 = fb_get_pixel(col + 1, row);
            uint32_t c2 = fb_get_pixel(col, row - 1);
            uint32_t c3 = fb_get_pixel(col, row + 1);

            uint32_t avg_r = (((c0 >> 16) & 0xFF) + ((c1 >> 16) & 0xFF) + ((c2 >> 16) & 0xFF) + ((c3 >> 16) & 0xFF)) / 4;
            uint32_t avg_g = (((c0 >> 8) & 0xFF)  + ((c1 >> 8) & 0xFF)  + ((c2 >> 8) & 0xFF)  + ((c3 >> 8) & 0xFF)) / 4;
            uint32_t avg_b = ((c0 & 0xFF) + (c1 & 0xFF) + (c2 & 0xFF) + (c3 & 0xFF)) / 4;

            fb_fill_rect(col, row, step, step, (avg_r << 16) | (avg_g << 8) | avg_b);
        }
    }
}

void fb_draw_acrylic_panel(int x, int y, int w, int h, int radius, uint32_t tint_rgb, uint8_t opacity)
{
    /* 1. Alttaki pikselleri buzlandir (Blur) */
    fb_blur_rect(x, y, w, h, 2);

    /* 2. Uzerine saydam Mica/Acrylic tonu uygula */
    fb_blend_round_rect(x, y, w, h, radius, tint_rgb, opacity);

    /* 3. 1px parildayan saydam kenarlik (Specular Highlight Border) */
    fb_blend_round_rect(x, y, w, 1, radius, 0xFFFFFF, 45);
    fb_blend_round_rect(x, y, 1, h, radius, 0xFFFFFF, 35);
    fb_blend_round_rect(x + w - 1, y, 1, h, radius, 0x000000, 50);
    fb_blend_round_rect(x, y + h - 1, w, 1, radius, 0x000000, 60);
}

/* High-Definition Sharp & Proportional Metin Çizimi */
void fb_draw_char(int x, int y, char c, uint32_t rgb, int scale)
{
    if (scale <= 0) scale = 1;
    unsigned char ch = (unsigned char)c;
    if (ch > 127) ch = '?';

    const uint8_t *glyph = font8x16[ch];
    uint8_t w = font8x16_width[ch];
    if (w > 8) w = 8;

    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < w; col++) {
            if (bits & (0x80 >> col)) {
                if (scale == 1) {
                    fb_put_pixel(x + col, y + row, rgb);
                } else {
                    fb_fill_rect(x + col * scale, y + row * scale, scale, scale, rgb);
                }
            }
        }
    }
}

void fb_draw_text(int x, int y, const char *text, uint32_t rgb, int scale)
{
    if (!text) return;
    if (scale <= 0) scale = 1;
    int cursor = x;
    while (*text) {
        unsigned char ch = (unsigned char)*text;
        if (ch > 127) ch = '?';
        fb_draw_char(cursor, y, ch, rgb, scale);
        uint8_t w = font8x16_width[ch];
        cursor += (int)(w + 1) * scale;
        text++;
    }
}

uint32_t fb_text_width(const char *text, int scale)
{
    if (!text) return 0;
    if (scale <= 0) scale = 1;
    uint32_t w = 0;
    while (*text) {
        unsigned char ch = (unsigned char)*text;
        if (ch > 127) ch = '?';
        w += (uint32_t)(font8x16_width[ch] + 1) * (uint32_t)scale;
        text++;
    }
    return w;
}

void fb_draw_text_centered(int cx, int y, const char *text, uint32_t rgb, int scale)
{
    uint32_t width = fb_text_width(text, scale);
    fb_draw_text(cx - (int)width / 2, y, text, rgb, scale);
}
