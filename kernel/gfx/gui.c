/*
 * Quantro OS - Masaustu Grafik Arayuzu (Windows 11 Fluent Design)
 */
#include "gui.h"
#include "fb.h"
#include "kernel.h"
#include "sound.h"

#include <stdint.h>

/* ---- Renk paleti (0xRRGGBB) -------------------------------------- */

static const uint32_t COL_WALLPAPER_TOP   = 0x05162C;
static const uint32_t COL_WALLPAPER_MID   = 0x0C3A63;
static const uint32_t COL_WALLPAPER_BOT   = 0x040D18;
static const uint32_t COL_GLOW            = 0x1B5F9B;
static const uint32_t COL_SHADOW          = 0x000000;
static const uint32_t COL_TASKBAR         = 0x102135;
static const uint32_t COL_TASKBAR_START   = 0x0078D4; /* Windows 11 Blue */
static const uint32_t COL_WINDOW_BODY     = 0x181F28; /* Mica Dark */
static const uint32_t COL_WINDOW_TITLE    = 0x202B37;
static const uint32_t COL_WINDOW_BORDER   = 0x3A4A5B;
static const uint32_t COL_WINDOW_TEXT     = 0xFFFFFF;
static const uint32_t COL_ACCENT          = 0x4CC2FF; /* Win11 Accent */
static const uint32_t COL_ACCENT_DARK     = 0x005FB8;
static const uint32_t COL_BUTTON_BG       = 0x2A3544;
static const uint32_t COL_BUTTON_BORDER   = 0x47586D;

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
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    return (uint32_t)(uint8_t)v;
}

static void draw_window_logo(int cx, int cy, int size, uint32_t light, uint32_t dark)
{
    /* Windows 11 4-kare pencere logosu */
    int half = size / 2;
    int gap = 2;
    int q = half - gap;

    fb_fill_round_rect(cx - half, cy - half, q, q, 2, light);
    fb_fill_round_rect(cx + gap,  cy - half, q, q, 2, dark);
    fb_fill_round_rect(cx - half, cy + gap,  q, q, 2, dark);
    fb_fill_round_rect(cx + gap,  cy + gap,  q, q, 2, light);
}

static void draw_start_icon(int cx, int cy, int size)
{
    draw_window_logo(cx, cy, size, 0x60CDFF, 0x0078D4);
}

static void draw_folder_icon(int cx, int cy, int size)
{
    int rim = size / 2;
    fb_fill_rect(cx - rim, cy - 6, 2 * rim, 4, 0xF5A623);
    fb_fill_round_rect(cx - rim + 2, cy - 2, 2 * (rim - 2), 2 * rim - 6, 4, 0xF7DC6F);
    fb_fill_rect(cx - rim, cy - 4, 2 * rim, 6, 0xF5A623);
}

static void draw_terminal_icon(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r + 2, cy - r + 2, 2 * r - 4, 2 * r - 4, 4, 0x1F2428);
    fb_fill_rect(cx - r + 4, cy - r + 4, 4, 4, COL_ACCENT);
    fb_fill_rect(cx - r + 10, cy - r + 4, 4, 4, 0x6E7681);
    fb_fill_rect(cx - r + 16, cy - r + 4, 4, 4, 0x6E7681);
    fb_draw_text(cx - 10, cy + 2, ">_", 0xFFFFFF, 2);
}

static void draw_settings_icon(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r + 2, cy - r + 2, 2 * r - 4, 2 * r - 4, 4, 0x30363D);
    fb_fill_round_rect(cx - 6, cy - 6, 12, 12, 3, COL_ACCENT);
}

static void draw_browser_icon(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r + 2, cy - r + 2, 2 * r - 4, 2 * r - 4, 6, 0x0078D4);
    fb_draw_text(cx - 5, cy - 6, "e", 0xFFFFFF, 2);
}

static void draw_calc_icon(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r + 2, cy - r + 2, 2 * r - 4, 2 * r - 4, 4, 0x24292E);
    fb_draw_text(cx - 7, cy - 7, "+-", COL_ACCENT, 1);
}

/* ---- Masaustu duvari --------------------------------------------- */

static void draw_glow(int cx, int cy, int radius, uint32_t color, uint8_t strength)
{
    if (!g_fb.ready) return;

    int x0 = cx - radius; int y0 = cy - radius;
    int x1 = cx + radius; int y1 = cy + radius;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int)g_w) x1 = (int)g_w;
    if (y1 > (int)g_h) y1 = (int)g_h;

    uint32_t r2 = (uint32_t)radius * radius;

    for (int yy = y0; yy < y1; yy++) {
        int dy = yy - cy;
        for (int xx = x0; xx < x1; xx++) {
            int dx = xx - cx;
            uint32_t d2 = (uint32_t)(dx * dx + dy * dy);
            if (d2 >= r2) continue;

            uint32_t alpha = (r2 - d2) * (uint32_t)strength / r2;
            if (alpha == 0) continue;

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
        uint32_t r = lerp_u8((COL_WALLPAPER_TOP >> 16) & 0xFF, (COL_WALLPAPER_BOT >> 16) & 0xFF, t);
        uint32_t gcol = lerp_u8((COL_WALLPAPER_TOP >> 8) & 0xFF, (COL_WALLPAPER_BOT >> 8) & 0xFF, t);
        uint32_t b = lerp_u8(COL_WALLPAPER_TOP & 0xFF, (COL_WALLPAPER_BOT & 0xFF), t);
        fb_fill_rect(0, (int)row, (int)g_w, 1, (r << 16) | (gcol << 8) | b);
    }

    draw_glow((int)(g_w * 3 / 4), (int)(g_h / 4), (int)(g_h / 2), COL_GLOW, 150);
    draw_glow((int)(g_w / 6), (int)(g_h * 4 / 5), (int)(g_h / 3), COL_GLOW, 100);
}

/* ---- Windows 11 Taskbar ------------------------------------------- */

static void draw_taskbar(void)
{
    int ty = (int)(g_h - g_taskbar_h);

    /* Taskbar arka plan (Acrylic blur efekti) */
    fb_fill_rect(0, ty, (int)g_w, (int)g_taskbar_h, COL_TASKBAR);
    fb_blend_rect(0, ty, (int)g_w, (int)g_taskbar_h, 0x000000, 70);
    fb_fill_rect(0, ty, (int)g_w, 1, COL_WINDOW_BORDER);

    /* Windows 11 Ortalanmis Gorev Cubugu */
    int icon_size = 38;
    int total_icons_w = 6 * 48;
    int center_start = (int)(g_w / 2) - (total_icons_w / 2);

    int sx = center_start;
    int sy = ty + ((int)g_taskbar_h - icon_size) / 2;

    /* 1. Baslat (Start) Logosu */
    fb_fill_round_rect(sx - 4, sy - 2, icon_size + 8, icon_size + 4, 6, 0x1B314B);
    draw_start_icon(sx + icon_size / 2, sy + icon_size / 2, 26);
    sx += 48;

    /* 2. Edge Tarayici */
    draw_browser_icon(sx + icon_size / 2, sy + icon_size / 2, 34);
    sx += 48;

    /* 3. Dosya Gezgini */
    draw_folder_icon(sx + icon_size / 2, sy + icon_size / 2, 34);
    sx += 48;

    /* 4. Terminal */
    draw_terminal_icon(sx + icon_size / 2, sy + icon_size / 2, 34);
    sx += 48;

    /* 5. Hesap Makinesi */
    draw_calc_icon(sx + icon_size / 2, sy + icon_size / 2, 34);
    sx += 48;

    /* 6. Ayarlar */
    draw_settings_icon(sx + icon_size / 2, sy + icon_size / 2, 34);

    /* Sol Taraf: Quantro OS Widget / Hava Durumu */
    fb_draw_text(20, ty + 18, "☀ 22 C Istanbul", 0xCFD8DC, 1);

    /* Sag Taraf: Sistem Tepsisi (Tray) */
    int rx = (int)g_w - 20;

    /* Saat & Tarih */
    char buf[64];
    ksnprintf(buf, sizeof(buf), "%02d:%02d",
              ((g_boot_time + g_uptime) / 3600) % 24,
              ((g_boot_time + g_uptime) / 60) % 60);
    uint32_t tw = fb_text_width(buf, 1);
    rx -= (int)tw + 10;
    fb_draw_text(rx, ty + 12, buf, 0xFFFFFF, 1);

    fb_draw_text(rx - 8, ty + 28, "25.10.2026", 0x90A4AE, 1);
    rx -= 30;

    /* Ses & Ag Simgeleri */
    fb_draw_text(rx - 60, ty + 20, "📶 🔊", COL_ACCENT, 1);
}

/* ---- Windows 11 Pencere ------------------------------------------- */

static void draw_window(void)
{
    int shadow = 12;

    /* Pencere Golgesi */
    fb_fill_round_rect(g_win_x + shadow, g_win_y + shadow,
                       (int)g_win_w, (int)g_win_h, 14, COL_SHADOW);
    fb_blend_round_rect(g_win_x + shadow, g_win_y + shadow,
                        (int)g_win_w, (int)g_win_h, 14, 0x000000, 140);

    /* Mica Govdesi */
    fb_fill_round_rect(g_win_x, g_win_y, (int)g_win_w, (int)g_win_h, 12, COL_WINDOW_BODY);
    fb_fill_round_rect(g_win_x, g_win_y, (int)g_win_w, 46, 12, COL_WINDOW_TITLE);
    fb_fill_rect(g_win_x, g_win_y + 36, (int)g_win_w, 10, COL_WINDOW_TITLE);

    /* Pencere Basligi ve Simgesi */
    draw_start_icon(g_win_x + 22, g_win_y + 23, 20);
    fb_draw_text(g_win_x + 50, g_win_y + 15, "Quantro OS - Windows 11 Sistem Ozellikleri",
                 COL_WINDOW_TEXT, 1);

    /* Pencere Kontrol Butonlari (Kapat, Buyut, Kucult) */
    int bx = (int)(g_win_x + g_win_w - 44);
    fb_fill_round_rect(bx, g_win_y + 10, 32, 26, 4, 0xC42B1C); /* Close (Kirmizi) */
    fb_draw_text(bx + 11, g_win_y + 16, "x", 0xFFFFFF, 1);

    bx -= 38;
    fb_fill_round_rect(bx, g_win_y + 10, 32, 26, 4, 0x2A3544); /* Maximize */
    fb_draw_text(bx + 10, g_win_y + 16, "[]", 0xCFD8DC, 1);

    bx -= 38;
    fb_fill_round_rect(bx, g_win_y + 10, 32, 26, 4, 0x2A3544); /* Minimize */
    fb_draw_text(bx + 12, g_win_y + 16, "-", 0xCFD8DC, 1);

    /* Govde Icerigi */
    int sy = g_win_y + 70;

    /* Sol buyuk Windows logosu */
    draw_start_icon(g_win_x + 50, sy + 50, 80);

    /* Baslik Bilgisi */
    fb_draw_text(g_win_x + 140, sy, "Quantro OS Pro (Windows 11 Edition)", COL_WINDOW_TEXT, 2);
    sy += 28;
    fb_draw_text(g_win_x + 140, sy, "Surum 24H2 (OS Derlemesi: 26100.1742 AArch64 Limine)", 0x90A4AE, 1);
    sy += 36;

    /* Ozellik Karti 1: Donanim */
    fb_fill_round_rect(g_win_x + 40, sy, (int)g_win_w - 80, 72, 8, 0x1F2937);
    fb_draw_text(g_win_x + 56, sy + 12, "Donanim & Cekirdek:", COL_ACCENT, 1);
    fb_draw_text(g_win_x + 56, sy + 30, "Islemci: ARM64 Cortex-A53 @ 1.50GHz (4 VCPU)", COL_WINDOW_TEXT, 1);
    fb_draw_text(g_win_x + 56, sy + 48, "Bellek:  1024 MB DDR4 / Ekran: 1280x800 TrueColor", COL_WINDOW_TEXT, 1);
    sy += 84;

    /* Ozellik Karti 2: Guvenlik & Standartlar */
    fb_fill_round_rect(g_win_x + 40, sy, (int)g_win_w - 80, 72, 8, 0x1F2937);
    fb_draw_text(g_win_x + 56, sy + 12, "Windows 11 Guvenlik Standartlari:", 0x66BB6A, 1);
    fb_draw_text(g_win_x + 56, sy + 30, "[✔] TPM 2.0 (PCR0-7 Validated)  [✔] UEFI Secure Boot Etkin", COL_WINDOW_TEXT, 1);
    fb_draw_text(g_win_x + 56, sy + 48, "[✔] VBS / HVCI Bellek Butunlugu [✔] Quantro Defender Aktif", COL_WINDOW_TEXT, 1);
    sy += 84;

    /* Ozellik Karti 3: Ag ve Alt Sistemler */
    fb_fill_round_rect(g_win_x + 40, sy, (int)g_win_w - 80, 68, 8, 0x1F2937);
    fb_draw_text(g_win_x + 56, sy + 10, "Ag & Depolama Durumu:", 0xFFB74D, 1);
    fb_draw_text(g_win_x + 56, sy + 28, "NDIS Adapter: Ethernet0 (192.168.1.105 / 1000 Mbps Link OK)", COL_WINDOW_TEXT, 1);
    fb_draw_text(g_win_x + 56, sy + 46, "Depolama: C:\\ (NTFS/VFS Sanal Disk) + FAT32 ESP Bolumu", COL_WINDOW_TEXT, 1);
    sy += 80;

    /* Butonlar */
    int bw = 140, bh = 34;
    int by = (int)g_win_y + (int)g_win_h - 50;

    fb_fill_round_rect(g_win_x + (int)g_win_w - bw - 160, by, bw, bh, 6, COL_BUTTON_BG);
    fb_draw_text_centered(g_win_x + (int)g_win_w - 90, by + 10, "Kapat", COL_WINDOW_TEXT, 1);

    fb_fill_round_rect(g_win_x + (int)g_win_w - bw - 10, by, bw, bh, 6, COL_TASKBAR_START);
    fb_draw_text_centered(g_win_x + (int)g_win_w + 60, by + 10, "Ayarlar...", COL_WINDOW_TEXT, 1);
}

/* ---- GUI API ------------------------------------------------------ */

void gui_init(void)
{
    g_w = fb_width();
    g_h = fb_height();
    if (g_w == 0 || g_h == 0)
        return;

    g_taskbar_h = 56;
    g_win_w = g_w > 960 ? 760 : (g_w * 8 / 10);
    g_win_h = g_h > 700 ? 540 : (g_h * 8 / 10);
    g_win_x = (int)g_w / 2 - (int)g_win_w / 2;
    g_win_y = (int)(g_h - g_taskbar_h) / 2 - (int)g_win_h / 2;
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
    ksnprintf(buf, sizeof(buf), "%02d:%02d",
              ((g_boot_time + g_uptime) / 3600) % 24,
              ((g_boot_time + g_uptime) / 60) % 60);

    uint32_t tw = fb_text_width(buf, 1);
    int bx = (int)g_w - 70;
    int by = ty + 12;

    fb_fill_rect(bx - 4, by - 2, (int)tw + 8, 18, COL_TASKBAR);
    fb_draw_text(bx, by, buf, 0xFFFFFF, 1);
}

void gui_panic(const char *message)
{
    /* Windows 11 Modern Mavi Ekran (BSOD) */
    fb_fill_rect(0, 0, (int)g_w, (int)g_h, 0x0078D4);
    fb_draw_text(90, 100, ":(", 0xFFFFFF, 8);
    fb_draw_text(90, 240, "Kisisel bilgisayariniz bir sorunla karsilasti ve yeniden baslatilmasi gerekiyor.", 0xFFFFFF, 2);
    fb_draw_text(90, 280, "Sadece bazi hata bilgileri toplaniyor, ardindan yeniden baslatilacak.", 0xFFFFFF, 1);
    fb_draw_text(90, 320, "Durdurma Kodu: CRITICAL_PROCESS_DIED", 0xFFFFFF, 1);
    fb_draw_text(90, 350, "Basarisiz Olan: ntoskrnl.exe", 0xFFFFFF, 1);
    fb_draw_text(90, 390, message, 0xFFEB3B, 1);
}
