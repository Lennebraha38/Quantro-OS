/*
 * Benim-OS Pro - Masaustu cizimi
 */
#include "gui.h"
#include "fb.h"
#include "kernel.h"

#include <stdint.h>

/* ---- Renk paleti (0xRRGGBB) -------------------------------------- */

static const uint32_t COL_WALLPAPER_TOP   = 0x05162C;
static const uint32_t COL_WALLPAPER_MID   = 0x0C3A63;
static const uint32_t COL_WALLPAPER_BOT   = 0x040D18;
static const uint32_t COL_GLOW            = 0x1B5F9B;
static const uint32_t COL_SHADOW          = 0x000000;
static const uint32_t COL_TASKBAR         = 0x0E2A47;
static const uint32_t COL_TASKBAR_START   = 0x1D6FB0;
static const uint32_t COL_WINDOW_BODY     = 0x1E1E1E;
static const uint32_t COL_WINDOW_TITLE    = 0x2A2A2A;
static const uint32_t COL_WINDOW_BORDER   = 0x3A3A3A;
static const uint32_t COL_WINDOW_TEXT     = 0xFFFFFF;
static const uint32_t COL_ACCENT          = 0x4CC2FF;
static const uint32_t COL_ACCENT_DARK     = 0x1E84C5;
static const uint32_t COL_BUTTON_BG       = 0x333333;
static const uint32_t COL_BUTTON_BORDER   = 0x555555;

/* ---- Layout ------------------------------------------------------- */

static uint32_t g_w = 0;
static uint32_t g_h = 0;

static int32_t g_win_x = 0;
static int32_t g_win_y = 0;
static uint32_t g_win_w = 0;
static uint32_t g_win_h = 0;

static uint32_t g_taskbar_h = 56;

static int64_t g_boot_time = 0;   /* Unix zaman damgasi */
static uint64_t g_uptime = 0;     /* Saniye */

/* ---- Yardimcilar -------------------------------------------------- */

static uint32_t lerp_u8(uint8_t a, uint8_t b, int t)
{
    int v = a + (int)(b - a) * t / 255;

    if (v < 0)
        v = 0;
    if (v > 255)
        v = 255;

    return (uint32_t)(uint8_t)v;
}

static void draw_window_logo(int cx, int cy, int size, uint32_t light,
                             uint32_t dark)
{
    /* Windows tarali 4-karelik logosu */
    int quarter = size / 4;

    fb_fill_rect(cx - quarter, cy - quarter, quarter, quarter, light);
    fb_fill_rect(cx, cy - quarter, quarter, quarter, dark);
    fb_fill_rect(cx - quarter, cy, quarter, quarter, dark);
    fb_fill_rect(cx, cy, quarter, quarter, light);
}

static void draw_start_icon(int cx, int cy, int size)
{
    draw_window_logo(cx, cy, size, 0xFFFFFF, COL_ACCENT);
}

static void draw_folder_icon(int cx, int cy, int size)
{
    int rim = size / 2;

    /* Klasov golevi */
    fb_fill_rect(cx - rim, cy - 4, 2 * rim, 4, 0xF5A623);
    /* Gövde */
    fb_fill_rect(cx - rim + 2, cy - 2, 2 * (rim - 2), 2 * rim - 4, 0xF7DC6F);
    fb_fill_rect(cx - rim, cy - 4, 2 * rim, 8, 0xF5A623);
}

static void draw_terminal_icon(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_rect(cx - r + 2, cy - r + 2, 2 * r - 4, 2 * r - 4, 0x2D2D2D);
    fb_fill_rect(cx - r + 4, cy - r + 4, 4, 4, COL_ACCENT);
    fb_fill_rect(cx - r + 10, cy - r + 4, 4, 4, 0x555555);
    fb_fill_rect(cx - r + 16, cy - r + 4, 4, 4, 0x555555);
    fb_draw_text(cx - 12, cy + 2, ">_", 0xFFFFFF, 2);
}

static void draw_settings_icon(int cx, int cy, int size)
{
    int r = size / 2;

    /* Basit cember + orta kare */
    for (int angle = 0; angle < 360; angle += 20) {
        int rad = angle * 314 / 180;
        int xx = cx + r * (1000 - 2 * ((rad * 7919) % 1000)) / 1000;
        int yy = cy + r * (1000 - 2 * ((rad * 13171) % 1000)) / 1000;
        fb_put_pixel(xx, yy, 0xFFFFFF);
    }
    fb_fill_rect(cx - 4, cy - 4, 8, 8, COL_ACCENT);
}

/* ---- Masaustu duvari --------------------------------------------- */

static void draw_glow(int cx, int cy, int radius, uint32_t color,
                      uint8_t strength)
{
    if (!g_fb.ready)
        return;

    int x0 = cx - radius;
    int y0 = cy - radius;
    int x1 = cx + radius;
    int y1 = cy + radius;

    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > (int)g_w)
        x1 = (int)g_w;
    if (y1 > (int)g_h)
        y1 = (int)g_h;

    uint32_t r2 = (uint32_t)radius * radius;

    for (int yy = y0; yy < y1; yy++) {
        int dy = yy - cy;
        for (int xx = x0; xx < x1; xx++) {
            int dx = xx - cx;
            uint32_t d2 = (uint32_t)(dx * dx + dy * dy);

            if (d2 >= r2)
                continue;

            uint32_t alpha = (r2 - d2) * (uint32_t)strength / r2;
            if (alpha == 0)
                continue;

            uint32_t dst = fb_get_pixel(xx, yy);
            uint32_t sr = (color >> 16) & 0xFF;
            uint32_t sg = (color >> 8) & 0xFF;
            uint32_t sb = color & 0xFF;
            uint32_t inv = 255u - alpha;
            uint32_t nr = (sr * alpha + ((dst >> 16) & 0xFF) * inv) / 255;
            uint32_t ng = (sg * alpha + ((dst >> 8) & 0xFF) * inv) / 255;
            uint32_t nb = (sb * alpha + (dst & 0xFF) * inv) / 255;
            fb_put_pixel(xx, yy, (nr << 16) | (ng << 8) | nb);
        }
    }
}

static void draw_wallpaper(void)
{
    for (uint32_t row = 0; row < g_h; row++) {
        int t = g_h > 1 ? (int)(row * 255 / (g_h - 1)) : 0;

        uint32_t r = lerp_u8((COL_WALLPAPER_TOP >> 16) & 0xFF,
                             (COL_WALLPAPER_BOT >> 16) & 0xFF, t);
        uint32_t gcol = lerp_u8((COL_WALLPAPER_TOP >> 8) & 0xFF,
                                (COL_WALLPAPER_BOT >> 8) & 0xFF, t);
        uint32_t b = lerp_u8(COL_WALLPAPER_TOP & 0xFF,
                             COL_WALLPAPER_BOT & 0xFF, t);

        fb_fill_rect(0, (int)row, (int)g_w, 1,
                     (r << 16) | (gcol << 8) | b);
    }

    draw_glow((int)(g_w * 3 / 4), (int)(g_h / 4), (int)(g_h / 2),
              COL_GLOW, 140);
    draw_glow((int)(g_w / 6), (int)(g_h * 4 / 5), (int)(g_h / 3),
              COL_GLOW, 90);
}

/* ---- Taskbar ------------------------------------------------------ */

static void draw_taskbar(void)
{
    int ty = (int)(g_h - g_taskbar_h);

    fb_fill_rect(0, ty, (int)g_w, (int)g_taskbar_h, COL_TASKBAR);
    fb_blend_rect(0, ty, (int)g_w, (int)g_taskbar_h, 0x000000, 60);

    /* Start butonu */
    int sw = 120;
    int sh = (int)g_taskbar_h - 16;
    int sx = 16;
    int sy = ty + 8;

    fb_fill_round_rect(sx, sy, sw, sh, 10, COL_TASKBAR_START);
    draw_start_icon(sx + sw / 2, sy + sh / 2, 40);
    fb_draw_text_centered(sx + sw / 2, sy + sh + 20, "Baslat",
                          0xFFFFFF, 1);

    /* Uygulama simgeleri */
    int cx = sx + sw + 30;
    draw_folder_icon(cx, sy + sh / 2, 40);
    cx += 56;
    draw_terminal_icon(cx, sy + sh / 2, 40);
    cx += 56;
    draw_settings_icon(cx, sy + sh / 2, 40);

    /* Sag taraf: saat */
    char buf[64];
    ksnprintf(buf, sizeof(buf), "%02d:%02d UTC",
              ((g_boot_time + g_uptime) / 3600) % 24,
              ((g_boot_time + g_uptime) / 60) % 60);
    uint32_t tw = fb_text_width(buf, 2);
    fb_draw_text((int)g_w - (int)tw - 40, ty + 8, buf, 0xFFFFFF, 2);

    /* Calisma suresi */
    ksnprintf(buf, sizeof(buf), "Calisma: %02d:%02d:%02d",
              (unsigned)(g_uptime / 3600),
              (unsigned)((g_uptime / 60) % 60),
              (unsigned)(g_uptime % 60));
    tw = fb_text_width(buf, 1);
    fb_draw_text((int)g_w - (int)tw - 300, ty + 24, buf, 0xFFFFFF, 1);
}

/* ---- Pencere ------------------------------------------------------ */

static void draw_window(void)
{
    int shadow = 10;

    /* Golge */
    fb_fill_round_rect(g_win_x + shadow, g_win_y + shadow,
                       (int)g_win_w, (int)g_win_h, 12, COL_SHADOW);
    fb_blend_round_rect(g_win_x + shadow, g_win_y + shadow,
                        (int)g_win_w, (int)g_win_h, 12, 0x000000, 110);

    /* Gövde */
    fb_fill_round_rect(g_win_x, g_win_y, (int)g_win_w, (int)g_win_h, 12,
                       COL_WINDOW_BODY);
    fb_blend_rect(g_win_x, g_win_y + 44, (int)g_win_w, (int)g_win_h - 44,
                  0x000000, 20);
    fb_blend_rect(g_win_x, g_win_y, (int)g_win_w, 1,
                  COL_WINDOW_BORDER, 100);

    /* Baslik cubugu */
    fb_fill_round_rect(g_win_x, g_win_y, (int)g_win_w, 44, 8,
                       COL_WINDOW_TITLE);
    fb_fill_rect(g_win_x, g_win_y + 36, (int)g_win_w, 8,
                 COL_WINDOW_TITLE);

    draw_start_icon(g_win_x + 16, g_win_y + 22, 24);
    fb_draw_text(g_win_x + 48, g_win_y + 14, "Benim-OS Hakkinda",
                 COL_WINDOW_TEXT, 1);

    /* Kullanim butonlari (sadece goruntuleme) */
    int bx = (int)(g_win_x + g_win_w - 40);
    fb_fill_rect(bx, g_win_y + 10, 16, 16, 0x888888);
    bx -= 24;
    fb_fill_rect(bx, g_win_y + 10, 16, 16, 0xAAAAAA);
    bx -= 24;
    fb_fill_rect(bx, g_win_y + 10, 16, 16, 0xFF5555);

    /* Icerik baslik */
    fb_draw_text(g_win_x + 40, g_win_y + 72, "Sistem Durumu",
                 COL_ACCENT, 1);

    /* Logo alani */
    draw_start_icon(g_win_x + 40, g_win_y + 110, 64);

    /* Metin satirlari */
    int sy = g_win_y + 100;
    fb_draw_text(g_win_x + 120, sy, "Benim-OS Pro", COL_WINDOW_TEXT, 2);
    sy += 30;
    fb_draw_text(g_win_x + 120, sy, "Surum: 0.2.0 (AArch64)",
                 COL_WINDOW_TEXT, 1);
    sy += 24;
    fb_draw_text(g_win_x + 120, sy, "Cekirdek: Limine Boot Protocol rev 2",
                 COL_WINDOW_TEXT, 1);
    sy += 24;
    fb_draw_text(g_win_x + 120, sy, "Ekran: 1280x800 x32",
                 COL_WINDOW_TEXT, 1);
    sy += 24;
    fb_draw_text(g_win_x + 120, sy, "Bellek: 1 GiB kullanilabilir",
                 COL_WINDOW_TEXT, 1);
    sy += 24;
    fb_draw_text(g_win_x + 120, sy, "Baslangic: UTC",
                 COL_WINDOW_TEXT, 1);
    sy += 24;
    fb_draw_text(g_win_x + 120, sy, "Calisma: 00:00:00",
                 COL_ACCENT, 1);
    sy += 36;

    /* Durum ve cubuk */
    fb_draw_text(g_win_x + 40, sy, "Durum: Tum sistemler calisiyor",
                 COL_WINDOW_TEXT, 1);
    sy += 24;
    fb_fill_rect(g_win_x + 40, sy, 320, 10, 0x333333);
    fb_fill_rect(g_win_x + 40, sy, 160, 10, COL_ACCENT);
    sy += 30;

    /* Butonlar */
    int bw = 120, bh = 34;
    int by = (int)g_win_y + (int)g_win_h - 60;

    fb_fill_round_rect(g_win_x + (int)g_win_w - bw - 200, by, bw, bh, 8,
                       COL_BUTTON_BG);
    fb_draw_text_centered(g_win_x + (int)g_win_w - 140, by + 10, "Tamam",
                          COL_WINDOW_TEXT, 1);

    fb_fill_round_rect(g_win_x + (int)g_win_w - bw - 40, by, bw, bh, 8,
                       COL_ACCENT_DARK);
    fb_draw_text_centered(g_win_x + (int)g_win_w - 20, by + 10,
                          "Yeniden Baslat", COL_WINDOW_TEXT, 1);
}

/* ---- GUI API ------------------------------------------------------ */

void gui_init(void)
{
    g_w = fb_width();
    g_h = fb_height();
    if (g_w == 0 || g_h == 0)
        return;

    g_taskbar_h = g_h / 10;
    if (g_taskbar_h < 48)
        g_taskbar_h = 48;

    g_win_w = g_w > 960 ? 700 : (g_w * 3 / 4);
    g_win_h = g_h * 3 / 4;
    g_win_x = (int)g_w / 2 - (int)g_win_w / 2;
    g_win_y = (int)g_h / 2 - (int)g_win_h / 2;
}

void gui_redraw_all(void)
{
    draw_wallpaper();
    draw_window();
    draw_taskbar();
}

void gui_tick(int64_t boot_time, uint64_t uptime_seconds)
{
    if (!g_fb.ready)
        return;

    g_boot_time = boot_time;
    g_uptime = uptime_seconds;

    int ty = (int)(g_h - g_taskbar_h);

    /* Saati yenile */
    char buf[32];
    ksnprintf(buf, sizeof(buf), "%02d:%02d UTC",
              ((g_boot_time + g_uptime) / 3600) % 24,
              ((g_boot_time + g_uptime) / 60) % 60);

    uint32_t tw = fb_text_width(buf, 2);
    int bx = (int)g_w - (int)tw - 40;
    int by = ty + 8;

    fb_fill_rect(bx - 2, by - 2, (int)tw + 4, 34, COL_TASKBAR);
    fb_blend_rect(bx - 2, by - 2, (int)tw + 4, 34, 0x000000, 60);
    fb_draw_text(bx, by, buf, 0xFFFFFF, 2);

    /* Calisma suresi */
    ksnprintf(buf, sizeof(buf), "Calisma: %02d:%02d:%02d",
              (unsigned)(g_uptime / 3600),
              (unsigned)((g_uptime / 60) % 60),
              (unsigned)(g_uptime % 60));

    tw = fb_text_width(buf, 1);
    bx = (int)g_w - (int)tw - 300;
    by = ty + 24;

    fb_fill_rect(bx - 2, by - 2, (int)tw + 4, 20, COL_TASKBAR);
    fb_blend_rect(bx - 2, by - 2, (int)tw + 4, 20, 0x000000, 60);
    fb_draw_text(bx, by, buf, 0xFFFFFF, 1);
}

void gui_panic(const char *message)
{
    fb_fill_rect(0, 0, (int)g_w, (int)g_h, 0x8B0000);
    fb_draw_text(64, g_h / 2 - 32, "SISTEM HATASI", 0xFFFFFF, 2);
    fb_draw_text(64, g_h / 2, message, 0xFFFFFF, 1);
}
