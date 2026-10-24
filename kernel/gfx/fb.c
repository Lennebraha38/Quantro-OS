/*
 * Benim-OS Pro - Framebuffer cizim katmani
 *
 * Limine'in sagladigi lineer framebuffer uzerine calisir. 32 bpp
 * RGB modu hedeflenir; renk bilesenleri mask/shift ile donusturulur.
 */
#include "fb.h"
#include "kernel.h"
#include "font8x8.h"
#include "limine.h"

struct fb_surface g_fb;

static uint32_t pack_color(uint32_t rgb)
{
    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8) & 0xFF;
    uint32_t b = rgb & 0xFF;
    uint32_t value = 0;

    if (g_fb.red_size && g_fb.red_size < 8)
        r >>= (8 - g_fb.red_size);
    if (g_fb.green_size && g_fb.green_size < 8)
        g >>= (8 - g_fb.green_size);
    if (g_fb.blue_size && g_fb.blue_size < 8)
        b >>= (8 - g_fb.blue_size);

    value |= (r & ((1u << g_fb.red_size) - 1)) << g_fb.red_shift;
    value |= (g & ((1u << g_fb.green_size) - 1)) << g_fb.green_shift;
    value |= (b & ((1u << g_fb.blue_size) - 1)) << g_fb.blue_shift;

    return value;
}

static uint32_t unpack_color(uint32_t pixel)
{
    uint32_t r = (pixel >> g_fb.red_shift) & ((1u << g_fb.red_size) - 1);
    uint32_t g = (pixel >> g_fb.green_shift) & ((1u << g_fb.green_size) - 1);
    uint32_t b = (pixel >> g_fb.blue_shift) & ((1u << g_fb.blue_size) - 1);

    if (g_fb.red_size < 8)
        r = (r << (8 - g_fb.red_size)) | (r >> (2 * g_fb.red_size - 8));
    if (g_fb.green_size < 8)
        g = (g << (8 - g_fb.green_size)) | (g >> (2 * g_fb.green_size - 8));
    if (g_fb.blue_size < 8)
        b = (b << (8 - g_fb.blue_size)) | (b >> (2 * g_fb.blue_size - 8));

    return (r << 16) | (g << 8) | b;
}

void fb_init(struct limine_framebuffer *lfb)
{
    if (!lfb)
        return;

    if (lfb->bpp != 32) {
        /* 32 bpp disindaki modlar bu surumde desteklenmiyor */
        return;
    }

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

int fb_ready(void)
{
    return g_fb.ready;
}

uint32_t fb_width(void)
{
    return g_fb.width;
}

uint32_t fb_get_pixel(int x, int y)
{
    if (!g_fb.ready)
        return 0;

    if (x < 0 || y < 0 || (uint32_t)x >= g_fb.width ||
        (uint32_t)y >= g_fb.height)
        return 0;

    return unpack_color(
        g_fb.pixels[(uint32_t)y * g_fb.pitch_pixels + (uint32_t)x]);
}

uint32_t fb_height(void)
{
    return g_fb.height;
}

void fb_put_pixel(int x, int y, uint32_t rgb)
{
    if (!g_fb.ready)
        return;

    if (x < 0 || y < 0 || (uint32_t)x >= g_fb.width ||
        (uint32_t)y >= g_fb.height)
        return;

    g_fb.pixels[(uint32_t)y * g_fb.pitch_pixels + (uint32_t)x] =
        pack_color(rgb);
}

void fb_fill_rect(int x, int y, int w, int h, uint32_t rgb)
{
    if (!g_fb.ready || w <= 0 || h <= 0)
        return;

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w;
    int y1 = y + h;

    if (x1 > (int)g_fb.width)
        x1 = (int)g_fb.width;
    if (y1 > (int)g_fb.height)
        y1 = (int)g_fb.height;
    if (x0 >= x1 || y0 >= y1)
        return;

    uint32_t pixel = pack_color(rgb);

    for (int row = y0; row < y1; row++) {
        volatile uint32_t *line =
            g_fb.pixels + (uint32_t)row * g_fb.pitch_pixels;
        for (int col = x0; col < x1; col++)
            line[col] = pixel;
    }
}

void fb_blend_rect(int x, int y, int w, int h, uint32_t rgb, uint8_t alpha)
{
    if (!g_fb.ready || w <= 0 || h <= 0 || alpha == 0)
        return;

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w;
    int y1 = y + h;

    if (x1 > (int)g_fb.width)
        x1 = (int)g_fb.width;
    if (y1 > (int)g_fb.height)
        y1 = (int)g_fb.height;
    if (x0 >= x1 || y0 >= y1)
        return;

    uint32_t src_r = (rgb >> 16) & 0xFF;
    uint32_t src_g = (rgb >> 8) & 0xFF;
    uint32_t src_b = rgb & 0xFF;
    uint32_t inv = 255u - alpha;

    for (int row = y0; row < y1; row++) {
        volatile uint32_t *line =
            g_fb.pixels + (uint32_t)row * g_fb.pitch_pixels;
        for (int col = x0; col < x1; col++) {
            uint32_t dst = unpack_color(line[col]);
            uint32_t r = ((src_r * alpha) + (((dst >> 16) & 0xFF) * inv)) / 255;
            uint32_t g = ((src_g * alpha) + (((dst >> 8) & 0xFF) * inv)) / 255;
            uint32_t b = ((src_b * alpha) + ((dst & 0xFF) * inv)) / 255;
            line[col] = pack_color((r << 16) | (g << 8) | b);
        }
    }
}

static int isqrt_int(int value)
{
    int root = 0;

    while ((root + 1) * (root + 1) <= value)
        root++;

    return root;
}

void fb_fill_round_rect(int x, int y, int w, int h, int radius, uint32_t rgb)
{
    if (w <= 0 || h <= 0)
        return;

    if (radius > w / 2)
        radius = w / 2;
    if (radius > h / 2)
        radius = h / 2;
    if (radius <= 0) {
        fb_fill_rect(x, y, w, h, rgb);
        return;
    }

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

void fb_blend_round_rect(int x, int y, int w, int h, int radius,
                         uint32_t rgb, uint8_t alpha)
{
    if (w <= 0 || h <= 0)
        return;

    if (radius > w / 2)
        radius = w / 2;
    if (radius > h / 2)
        radius = h / 2;
    if (radius <= 0) {
        fb_blend_rect(x, y, w, h, rgb, alpha);
        return;
    }

    fb_blend_rect(x + radius, y, w - 2 * radius, h, rgb, alpha);
    fb_blend_rect(x, y + radius, radius, h - 2 * radius, rgb, alpha);
    fb_blend_rect(x + w - radius, y + radius, radius, h - 2 * radius, rgb,
                  alpha);

    for (int i = 0; i < radius; i++) {
        int dy = radius - i;
        int dx = radius - isqrt_int(radius * radius - dy * dy);

        fb_blend_rect(x + dx, y + i, w - 2 * dx, 1, rgb, alpha);
        fb_blend_rect(x + dx, y + h - 1 - i, w - 2 * dx, 1, rgb, alpha);
    }
}

void fb_gradient_rect(int x, int y, int w, int h, uint32_t top_rgb,
                      uint32_t bottom_rgb)
{
    if (!g_fb.ready || w <= 0 || h <= 0)
        return;

    int tr = (top_rgb >> 16) & 0xFF, tg = (top_rgb >> 8) & 0xFF,
        tb = top_rgb & 0xFF;
    int br = (bottom_rgb >> 16) & 0xFF, bg = (bottom_rgb >> 8) & 0xFF,
        bb = bottom_rgb & 0xFF;

    for (int row = 0; row < h; row++) {
        int t = h > 1 ? row * 255 / (h - 1) : 0;
        uint32_t r = (uint32_t)(tr + (br - tr) * t / 255);
        uint32_t g = (uint32_t)(tg + (bg - tg) * t / 255);
        uint32_t b = (uint32_t)(tb + (bb - tb) * t / 255);
        fb_fill_rect(x, y + row, w, 1, (r << 16) | (g << 8) | b);
    }
}

void fb_draw_char(int x, int y, char c, uint32_t rgb, int scale)
{
    if (scale <= 0)
        scale = 1;
    unsigned char ch = (unsigned char)c;
    if (ch > 127)
        ch = '?';

    const uint8_t *glyph = font8x8[ch];

    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1u << col))
                fb_fill_rect(x + col * scale, y + row * scale, scale, scale,
                             rgb);
        }
    }
}

void fb_draw_text(int x, int y, const char *text, uint32_t rgb, int scale)
{
    if (!text)
        return;

    int cursor = x;
    while (*text) {
        fb_draw_char(cursor, y, *text, rgb, scale);
        cursor += 8 * scale;
        text++;
    }
}

uint32_t fb_text_width(const char *text, int scale)
{
    if (!text)
        return 0;
    return (uint32_t)strlen(text) * 8u * (uint32_t)scale;
}

void fb_draw_text_centered(int cx, int y, const char *text, uint32_t rgb,
                           int scale)
{
    uint32_t width = fb_text_width(text, scale);
    fb_draw_text(cx - (int)width / 2, y, text, rgb, scale);
}
