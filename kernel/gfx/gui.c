/*
 * Quantro OS - Ultra Realistic Windows 11 Fluent Design & Mica GUI Motoru
 * - Fotogercekci Procedural "Bloom" Soyut Duvar Kagidi
 * - Buzlu Cam Acrylic / Mica Saydam Paneller
 * - Ortalanmis Windows 11 Taskbar & Tepsi
 * - Gelismis Baslat Menusu (Start Menu Popup) & 3x6 Sabitlenen Uygulamalar
 * - Windows 11 Sistem & Ayarlar Penceresi
 * - Windows 11 Aero Mouse Cursor
 */
#include "gui.h"
#include "fb.h"
#include "kernel.h"
#include "sound.h"
#include "pmm.h"
#include "heap.h"

#include <stdint.h>

#define RGB(r, g, b)        (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

/* Windows 11 Orijinal Renk Paleti */
#define CLR_BLOOM_BG_TOP    0x020813
#define CLR_BLOOM_BG_BOT    0x0A1B33
#define CLR_BLOOM_PETAL1    0x0078D4
#define CLR_BLOOM_PETAL2    0x00A4EF
#define CLR_BLOOM_PETAL3    0x2899F5
#define CLR_BLOOM_PETAL4    0x60CDFF
#define CLR_BLOOM_GLOW      0xBAE6FD
#define CLR_BLOOM_PURPLE    0x5B21B6

#define CLR_MICA_BG         0x202020
#define CLR_MICA_CARD       0x2B2B2B
#define CLR_MICA_BORDER     0x3D3D3D
#define CLR_TEXT_WHITE      0xFFFFFF
#define CLR_TEXT_GRAY       0xCCCCCC
#define CLR_TEXT_MUTED      0x888888
#define CLR_ACCENT_BLUE     0x0078D4
#define CLR_ACCENT_CYAN     0x60CDFF

static uint32_t g_w = 1280;
static uint32_t g_h = 800;

static int64_t  g_boot_time = 0;
static uint64_t g_uptime = 0;

/* ---- Ikon Cizicileri ---------------------------------------------- */

static void draw_win_logo(int cx, int cy, int size)
{
    int half = size / 2;
    int gap = size > 20 ? 3 : 2;
    int q = half - gap;

    fb_fill_round_rect(cx - half, cy - half, q, q, 2, 0x00A4EF);
    fb_fill_round_rect(cx + gap,  cy - half, q, q, 2, 0x0078D4);
    fb_fill_round_rect(cx - half, cy + gap,  q, q, 2, 0x0063B1);
    fb_fill_round_rect(cx + gap,  cy + gap,  q, q, 2, 0x2899F5);
}

static void draw_fluent_edge(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, r, 0x0B55A0);
    fb_fill_round_rect(cx - r + 3, cy - r + 3, size - 6, size - 6, r - 3, 0x0078D4);
    fb_fill_round_rect(cx - r + 6, cy - r + 4, size - 10, r, r / 2, 0x00D2FF);
    fb_draw_text(cx - 5, cy - 6, "e", 0xFFFFFF, 2);
}

static void draw_fluent_explorer(int cx, int cy, int size)
{
    int w = size;
    int h = size * 3 / 4;
    int x = cx - w / 2;
    int y = cy - h / 2;

    /* Arka klasor kulagi */
    fb_fill_round_rect(x, y - 4, w / 2, 8, 3, 0x005FB8);
    /* Mavi klasor govdesi */
    fb_fill_round_rect(x, y, w, h, 4, 0x0078D4);
    /* On sari kartvizit cebi */
    fb_fill_round_rect(x + 2, y + 4, w - 4, h - 6, 3, 0xFFA000);
    fb_fill_round_rect(x + 2, y + 8, w - 4, h - 10, 3, 0xFFC107);
}

static void draw_fluent_terminal(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, 5, 0x111111);
    fb_fill_round_rect(cx - r + 1, cy - r + 1, size - 2, size - 2, 4, 0x1E1E1E);
    fb_fill_rect(cx - r + 4, cy - r + 3, size - 8, 2, 0x60CDFF);
    fb_draw_text(cx - 9, cy - 2, ">_", 0xFFFFFF, 1);
}

static void draw_fluent_settings(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, 6, 0x333333);
    fb_fill_round_rect(cx - 6, cy - 6, 12, 12, 3, 0x0078D4);
    fb_fill_round_rect(cx - 2, cy - 2, 4, 4, 1, 0xFFFFFF);
}

static void draw_fluent_store(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r + 4, size, size - 6, 4, 0x242424);
    fb_fill_round_rect(cx - r / 2, cy - r, r, 6, 3, 0x0078D4);
    draw_win_logo(cx, cy + 2, 10);
}

static void draw_fluent_calc(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, 5, 0x2A2A2A);
    fb_fill_round_rect(cx - r + 3, cy - r + 3, size - 6, 7, 2, 0x3A3A3A);
    fb_draw_text(cx - 8, cy - 1, "+ -", 0x60CDFF, 1);
}

/* ---- Fotogerçekçi Windows 11 "Bloom" Duvar Kağıdı Motoru ----------- */

static void draw_photorealistic_bloom(void)
{
    /* 1. Ultra Pürüzsüz Gece Mavisi Arka Plan */
    for (uint32_t y = 0; y < g_h; y++) {
        uint32_t t = (y * 255) / (g_h ? g_h : 1);
        uint32_t r = 2 + (t * 8) / 255;
        uint32_t g = 8 + (t * 19) / 255;
        uint32_t b = 19 + (t * 32) / 255;
        fb_fill_rect(0, (int)y, (int)g_w, 1, RGB(r, g, b));
    }

    int cx = (int)g_w / 2;
    int cy = (int)g_h / 2 - 20;

    /* 2. Arka Plan Mor/İndigo Parıltısı */
    for (int r = 360; r > 40; r -= 10) {
        uint8_t a = (uint8_t)((360 - r) * 45 / 360);
        fb_blend_round_rect(cx - r + 40, cy - r * 3 / 5, r * 2, r * 6 / 5, r / 2,
                            CLR_BLOOM_PURPLE, a);
    }

    /* 3. Ana Mavi Bloom Işık Küresi */
    for (int r = 300; r > 30; r -= 8) {
        uint8_t a = (uint8_t)((300 - r) * 70 / 300);
        fb_blend_round_rect(cx - r, cy - r * 2 / 3, r * 2, r * 4 / 3, r / 2,
                            CLR_BLOOM_PETAL1, a);
    }

    /* 4. Sol Katmanlı İpeksi Yapraklar (Cyan / Sky Waves) */
    for (int i = 0; i < 50; i++) {
        int px = cx - 220 + i * 5;
        int py = cy + 60 - i * 6;
        int pw = 140 + i * 2;
        int ph = 200 - i * 2;
        fb_blend_round_rect(px, py, pw, ph, 55, CLR_BLOOM_PETAL2, 6);
        fb_blend_round_rect(px + 10, py - 10, pw - 20, ph - 20, 45, CLR_BLOOM_PETAL4, 5);
    }

    /* 5. Sağ Katmanlı İpeksi Yapraklar (Deep Blue / Electric Petals) */
    for (int i = 0; i < 55; i++) {
        int px = cx - 30 + i * 5;
        int py = cy + 80 - i * 5;
        int pw = 150 + i * 2;
        int ph = 210 - i * 2;
        fb_blend_round_rect(px, py, pw, ph, 60, CLR_BLOOM_PETAL3, 6);
        fb_blend_round_rect(px - 10, py - 5, pw - 20, ph - 15, 50, CLR_BLOOM_PETAL1, 7);
    }

    /* 6. Merkez Odak Işıltısı (Bright Cyan Core) */
    for (int r = 160; r > 10; r -= 6) {
        uint8_t a = (uint8_t)((160 - r) * 95 / 160);
        fb_blend_round_rect(cx - r * 3 / 4, cy - r / 2, r * 3 / 2, r, r / 2,
                            CLR_BLOOM_GLOW, a);
    }
}

/* ---- Masaüstü Simgeleri ------------------------------------------- */

static void draw_desktop_icons(void)
{
    struct icon_desc {
        int type;
        const char *name;
    } icons[] = {
        { 0, "Bu Bilgisayar" },
        { 1, "Geri Donusum" },
        { 2, "Microsoft Edge" },
        { 3, "Windows Terminal" },
        { 4, "Dosyalarim" }
    };

    int start_x = 24;
    int start_y = 24;
    int step_y = 90;

    for (int i = 0; i < 5; i++) {
        int y = start_y + i * step_y;

        /* Hover arkaligi */
        fb_blend_round_rect(start_x - 6, y - 6, 84, 80, 6, 0xFFFFFF, 12);

        int cx = start_x + 36;
        int cy = y + 26;

        if (icons[i].type == 0) {
            /* Bilgisayar */
            fb_fill_round_rect(cx - 16, cy - 12, 32, 22, 3, 0x455A64);
            fb_fill_rect(cx - 13, cy - 9, 26, 16, 0x81D4FA);
            fb_fill_rect(cx - 4, cy + 10, 8, 4, 0x90A4AE);
            fb_fill_rect(cx - 8, cy + 14, 16, 2, 0x90A4AE);
        } else if (icons[i].type == 1) {
            /* Geri Donusum */
            fb_fill_round_rect(cx - 12, cy - 14, 24, 28, 4, 0x546E7A);
            fb_fill_rect(cx - 8, cy - 10, 16, 22, 0xB0BEC5);
            fb_draw_text(cx - 5, cy - 4, "♻", 0xFFFFFF, 1);
        } else if (icons[i].type == 2) {
            /* Edge */
            draw_fluent_edge(cx, cy, 32);
        } else if (icons[i].type == 3) {
            /* Terminal */
            draw_fluent_terminal(cx, cy, 32);
        } else {
            /* Dosyalarim */
            draw_fluent_explorer(cx, cy, 34);
        }

        fb_draw_text_centered(cx, y + 56, icons[i].name, CLR_TEXT_WHITE, 1);
    }
}

/* ---- Windows 11 Başlat Menüsü (Start Menu Popup) ------------------- */

static void draw_win11_start_menu(int cx, int bottom_y)
{
    int mw = 580;
    int mh = 500;
    int mx = cx - mw / 2;
    int my = bottom_y - mh - 12;

    /* 1. Derin Pencere Gölgesi */
    fb_blend_round_rect(mx - 10, my - 8, mw + 20, mh + 20, 18, 0x000000, 160);
    fb_blend_round_rect(mx - 4,  my - 3, mw + 8,  mh + 8,  16, 0x000000, 190);

    /* 2. Gerçek Acrylic / Mica Buzlu Cam Paneli */
    fb_draw_acrylic_panel(mx, my, mw, mh, 12, CLR_MICA_BG, 235);

    /* 3. Arama Kutusu (Search Capsule) */
    int sx = mx + 36;
    int sy = my + 28;
    int sw = mw - 72;
    int sh = 36;
    fb_fill_round_rect(sx, sy, sw, sh, 18, 0x1C1C1C);
    fb_blend_round_rect(sx, sy, sw, 1, 1, 0xFFFFFF, 30);
    fb_draw_text(sx + 16, sy + 10, "🔍 Aramak icin buraya yazin...", CLR_TEXT_MUTED, 1);

    /* 4. Sabitlenenler Başlığı */
    int py = sy + 52;
    fb_draw_text(mx + 38, py, "Sabitlenenler", CLR_TEXT_WHITE, 1);
    fb_draw_text(mx + mw - 160, py, "Tum uygulamalar >", CLR_ACCENT_CYAN, 1);

    /* 5. 3x6 Sabitlenen Uygulamalar */
    int app_x0 = mx + 48;
    int app_y0 = py + 26;
    int step_col = (mw - 96) / 6;

    /* Satir 1 */
    draw_fluent_edge(app_x0 + 0 * step_col, app_y0, 28);
    fb_draw_text_centered(app_x0 + 0 * step_col, app_y0 + 20, "Edge", CLR_TEXT_WHITE, 1);

    draw_fluent_explorer(app_x0 + 1 * step_col, app_y0, 28);
    fb_draw_text_centered(app_x0 + 1 * step_col, app_y0 + 20, "Gezgin", CLR_TEXT_WHITE, 1);

    draw_fluent_store(app_x0 + 2 * step_col, app_y0, 28);
    fb_draw_text_centered(app_x0 + 2 * step_col, app_y0 + 20, "Store", CLR_TEXT_WHITE, 1);

    draw_fluent_settings(app_x0 + 3 * step_col, app_y0, 28);
    fb_draw_text_centered(app_x0 + 3 * step_col, app_y0 + 20, "Ayarlar", CLR_TEXT_WHITE, 1);

    draw_fluent_calc(app_x0 + 4 * step_col, app_y0, 28);
    fb_draw_text_centered(app_x0 + 4 * step_col, app_y0 + 20, "Hesap", CLR_TEXT_WHITE, 1);

    draw_fluent_terminal(app_x0 + 5 * step_col, app_y0, 28);
    fb_draw_text_centered(app_x0 + 5 * step_col, app_y0 + 20, "Terminal", CLR_TEXT_WHITE, 1);

    /* Satir 2 */
    int row2_y = app_y0 + 54;
    fb_fill_round_rect(app_x0 + 0 * step_col - 14, row2_y - 14, 28, 28, 5, 0x185ABD);
    fb_draw_text_centered(app_x0 + 0 * step_col, row2_y - 5, "W", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x0 + 0 * step_col, row2_y + 18, "Word", CLR_TEXT_WHITE, 1);

    fb_fill_round_rect(app_x0 + 1 * step_col - 14, row2_y - 14, 28, 28, 5, 0x107C41);
    fb_draw_text_centered(app_x0 + 1 * step_col, row2_y - 5, "X", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x0 + 1 * step_col, row2_y + 18, "Excel", CLR_TEXT_WHITE, 1);

    fb_fill_round_rect(app_x0 + 2 * step_col - 14, row2_y - 14, 28, 28, 5, 0xC43E1C);
    fb_draw_text_centered(app_x0 + 2 * step_col, row2_y - 5, "P", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x0 + 2 * step_col, row2_y + 18, "P.Point", CLR_TEXT_WHITE, 1);

    fb_fill_round_rect(app_x0 + 3 * step_col - 14, row2_y - 14, 28, 28, 5, 0x0078D4);
    fb_draw_text_centered(app_x0 + 3 * step_col, row2_y - 5, "✉", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x0 + 3 * step_col, row2_y + 18, "Posta", CLR_TEXT_WHITE, 1);

    fb_fill_round_rect(app_x0 + 4 * step_col - 14, row2_y - 14, 28, 28, 5, 0xE74C3C);
    fb_draw_text_centered(app_x0 + 4 * step_col, row2_y - 5, "📅", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x0 + 4 * step_col, row2_y + 18, "Takvim", CLR_TEXT_WHITE, 1);

    fb_fill_round_rect(app_x0 + 5 * step_col - 14, row2_y - 14, 28, 28, 5, 0x2C3E50);
    fb_draw_text_centered(app_x0 + 5 * step_col, row2_y - 5, "📝", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x0 + 5 * step_col, row2_y + 18, "Notlar", CLR_TEXT_WHITE, 1);

    /* 6. Önerilenler */
    int rec_y = row2_y + 54;
    fb_draw_text(mx + 38, rec_y, "Onerilenler", CLR_TEXT_WHITE, 1);

    int c1_y = rec_y + 20;
    draw_fluent_explorer(mx + 54, c1_y + 10, 20);
    fb_draw_text(mx + 76, c1_y + 4, "welcome.txt", CLR_TEXT_WHITE, 1);
    fb_draw_text(mx + 76, c1_y + 18, "C:\\Users\\Quantro\\Desktop", CLR_TEXT_MUTED, 1);

    int c2_x = mx + mw / 2 + 10;
    fb_fill_round_rect(c2_x, c1_y + 2, 20, 20, 4, 0x0078D4);
    fb_draw_text(c2_x + 5, c1_y + 6, "NT", 0xFFFFFF, 1);
    fb_draw_text(c2_x + 30, c1_y + 4, "ntoskrnl.exe", CLR_TEXT_WHITE, 1);
    fb_draw_text(c2_x + 30, c1_y + 18, "C:\\Windows\\System32", CLR_TEXT_MUTED, 1);

    /* 7. Alt Kullanıcı Çubuğu */
    int foot_y = my + mh - 58;
    fb_fill_round_rect(mx, foot_y, mw, 58, 12, 0x181818);
    fb_blend_round_rect(mx, foot_y, mw, 1, 1, 0xFFFFFF, 25);

    /* Profil Resmi */
    fb_fill_round_rect(mx + 32, foot_y + 14, 30, 30, 15, CLR_ACCENT_BLUE);
    fb_draw_text(mx + 42, foot_y + 20, "Q", 0xFFFFFF, 1);
    fb_draw_text(mx + 72, foot_y + 18, "QuantroUser (Yonetici)", CLR_TEXT_WHITE, 1);
    fb_draw_text(mx + 72, foot_y + 32, "quantro@live.com", CLR_TEXT_MUTED, 1);

    /* Güç Düğmesi */
    int pwr_x = mx + mw - 56;
    fb_fill_round_rect(pwr_x, foot_y + 14, 32, 32, 6, 0x242424);
    fb_draw_text(pwr_x + 10, foot_y + 22, "⏻", 0xFF5555, 1);
}

/* ---- Windows 11 Modern Görev Çubuğu ------------------------------- */

static void draw_win11_taskbar(int start_open)
{
    int th = 50;
    int ty = (int)g_h - th;

    /* Buzlu Cam Acrylic Taskbar */
    fb_draw_acrylic_panel(0, ty, (int)g_w, th, 0, 0x161616, 230);

    /* Sol: Hava Durumu Widget'i */
    fb_fill_round_rect(16, ty + 8, 140, 34, 6, 0x222222);
    fb_blend_round_rect(16, ty + 8, 140, 1, 1, 0xFFFFFF, 20);
    fb_draw_text(26, ty + 16, "☀ 21 C Bulutlu", CLR_TEXT_WHITE, 1);

    /* Ortalanmış Uygulamalar */
    int count = 7;
    int box_sz = 42;
    int start_x = (int)(g_w / 2) - ((count * box_sz) / 2);
    int cur_x = start_x;

    /* 1. Başlat */
    if (start_open) {
        fb_fill_round_rect(cur_x - 2, ty + 4, box_sz, box_sz, 6, 0x2A2A2A);
        fb_fill_round_rect(cur_x + 12, ty + th - 4, 14, 3, 1, CLR_ACCENT_BLUE);
    }
    draw_win_logo(cur_x + box_sz / 2, ty + th / 2, 24);
    cur_x += box_sz;

    /* 2. Edge */
    draw_fluent_edge(cur_x + box_sz / 2, ty + th / 2, 28);
    fb_fill_round_rect(cur_x + 16, ty + th - 4, 6, 3, 1, CLR_TEXT_MUTED);
    cur_x += box_sz;

    /* 3. Explorer */
    draw_fluent_explorer(cur_x + box_sz / 2, ty + th / 2, 28);
    fb_fill_round_rect(cur_x + 12, ty + th - 4, 14, 3, 1, CLR_ACCENT_BLUE);
    cur_x += box_sz;

    /* 4. Store */
    draw_fluent_store(cur_x + box_sz / 2, ty + th / 2, 26);
    cur_x += box_sz;

    /* 5. Terminal */
    draw_fluent_terminal(cur_x + box_sz / 2, ty + th / 2, 26);
    cur_x += box_sz;

    /* 6. Calc */
    draw_fluent_calc(cur_x + box_sz / 2, ty + th / 2, 26);
    cur_x += box_sz;

    /* 7. Settings */
    draw_fluent_settings(cur_x + box_sz / 2, ty + th / 2, 26);
    fb_fill_round_rect(cur_x + 12, ty + th - 4, 14, 3, 1, CLR_ACCENT_BLUE);

    /* Sağ: Sistem Tepsisi */
    int rx = (int)g_w - 24;

    char time_buf[16], date_buf[16];
    ksnprintf(time_buf, sizeof(time_buf), "%02d:%02d",
              ((g_boot_time + g_uptime) / 3600) % 24,
              ((g_boot_time + g_uptime) / 60) % 60);
    ksnprintf(date_buf, sizeof(date_buf), "25.10.2026");

    uint32_t tw = fb_text_width(date_buf, 1);
    rx -= (int)tw + 12;

    fb_draw_text(rx + 16, ty + 10, time_buf, CLR_TEXT_WHITE, 1);
    fb_draw_text(rx, ty + 26, date_buf, CLR_TEXT_MUTED, 1);

    rx -= 100;
    fb_fill_round_rect(rx, ty + 8, 92, 34, 17, 0x222222);
    fb_blend_round_rect(rx, ty + 8, 92, 1, 1, 0xFFFFFF, 20);
    fb_draw_text(rx + 12, ty + 16, "📶 🔊 🔋", CLR_TEXT_WHITE, 1);
}

/* ---- Windows 11 Sistem & Ayarlar Penceresi ------------------------- */

static void draw_win11_settings_window(int wx, int wy, int ww, int wh)
{
    /* Gölge */
    fb_blend_round_rect(wx - 10, wy - 8, ww + 20, wh + 20, 18, 0x000000, 160);

    /* Acrylic Buzlu Cam Gövde */
    fb_draw_acrylic_panel(wx, wy, ww, wh, 10, CLR_MICA_BG, 240);

    /* Başlık Çubuğu */
    fb_fill_round_rect(wx, wy, ww, 42, 10, 0x242424);
    fb_fill_rect(wx, wy + 32, ww, 10, 0x242424);

    draw_fluent_settings(wx + 22, wy + 21, 20);
    fb_draw_text(wx + 46, wy + 15, "Ayarlar - Sistem Hakkinda", CLR_TEXT_WHITE, 1);

    /* Pencere Butonları */
    int bx = wx + ww - 46;
    fb_fill_round_rect(bx, wy + 7, 38, 28, 4, 0x242424);
    fb_draw_text(bx + 14, wy + 13, "✕", CLR_TEXT_WHITE, 1);

    bx -= 42;
    fb_fill_round_rect(bx, wy + 7, 38, 28, 4, 0x242424);
    fb_draw_text(bx + 13, wy + 13, "□", CLR_TEXT_WHITE, 1);

    bx -= 42;
    fb_fill_round_rect(bx, wy + 7, 38, 28, 4, 0x242424);
    fb_draw_text(bx + 13, wy + 13, "—", CLR_TEXT_WHITE, 1);

    /* Sol Menü */
    int sb_w = 180;
    int sb_x = wx;
    int sb_y = wy + 42;
    int sb_h = wh - 42;
    fb_fill_rect(sb_x, sb_y, sb_w, sb_h, 0x222222);
    fb_blend_round_rect(sb_x + sb_w - 1, sb_y, 1, sb_h, 1, 0xFFFFFF, 20);

    int my = sb_y + 16;
    fb_fill_round_rect(sb_x + 10, my - 4, sb_w - 20, 32, 6, 0x333333);
    fb_fill_round_rect(sb_x + 10, my + 4, 3, 16, 1, CLR_ACCENT_BLUE);
    fb_draw_text(sb_x + 24, my + 4, "🖥 Sistem", CLR_TEXT_WHITE, 1);
    my += 36;

    fb_draw_text(sb_x + 24, my + 4, "📶 Ag & Internet", CLR_TEXT_GRAY, 1);
    my += 34;

    fb_draw_text(sb_x + 24, my + 4, "🎨 Kisisellestir", CLR_TEXT_GRAY, 1);
    my += 34;

    fb_draw_text(sb_x + 24, my + 4, "🔒 Guvenlik & TPM", CLR_TEXT_GRAY, 1);
    my += 34;

    fb_draw_text(sb_x + 24, my + 4, "⚙ Windows Update", CLR_TEXT_GRAY, 1);

    /* Sağ İçerik */
    int cx = sb_x + sb_w + 24;
    int cy = sb_y + 16;

    draw_win_logo(cx + 24, cy + 24, 48);
    fb_draw_text(cx + 64, cy + 12, "QUANTRO-PC (ARM64 Virt)", CLR_TEXT_WHITE, 2);
    fb_draw_text(cx + 64, cy + 34, "Windows 11 Pro 24H2 | OS Derlemesi 26100.1742", CLR_TEXT_MUTED, 1);
    cy += 70;

    int cw = ww - sb_w - 48;
    /* Kart 1 */
    fb_fill_round_rect(cx, cy, cw, 78, 8, CLR_MICA_CARD);
    fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 25);
    fb_draw_text(cx + 16, cy + 10, "Cihaz Ozellikleri", CLR_ACCENT_CYAN, 1);
    fb_draw_text(cx + 16, cy + 30, "Islemci:       ARM Cortex-A53 @ 1.50 GHz (4 vCPU AArch64)", CLR_TEXT_WHITE, 1);
    fb_draw_text(cx + 16, cy + 46, "Bellek:        1024 MB RAM (PMM 4KB Granule + 2MB Kernel Heap)", CLR_TEXT_WHITE, 1);
    fb_draw_text(cx + 16, cy + 62, "Sistem Turu:   64-bit Isletim Sistemi, ARM tabanli islemci", CLR_TEXT_GRAY, 1);
    cy += 88;

    /* Kart 2 */
    fb_fill_round_rect(cx, cy, cw, 78, 8, CLR_MICA_CARD);
    fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 25);
    fb_draw_text(cx + 16, cy + 10, "Donanim & Guvenlik Standartlari", 0x66BB6A, 1);
    fb_draw_text(cx + 16, cy + 30, "[✔] TPM 2.0:        Etkin & Kilitli (PCR0-7 Donanim Dogrulamasi)", CLR_TEXT_WHITE, 1);
    fb_draw_text(cx + 16, cy + 46, "[✔] Secure Boot:    UEFI Platform Key (PK) Onayli", CLR_TEXT_WHITE, 1);
    fb_draw_text(cx + 16, cy + 62, "[✔] Bellek Korumasi: VBS + HVCI (W^X Enforced)", CLR_TEXT_GRAY, 1);
    cy += 88;

    /* Kart 3 */
    fb_fill_round_rect(cx, cy, cw, 68, 8, CLR_MICA_CARD);
    fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 25);
    fb_draw_text(cx + 16, cy + 10, "Ag & Depolama Durumu", 0xFFA726, 1);
    fb_draw_text(cx + 16, cy + 30, "NDIS Baglantisi: 192.168.1.105 (VirtIO 10Gbps Ethernet0 - Link UP)", CLR_TEXT_WHITE, 1);
    fb_draw_text(cx + 16, cy + 48, "Disk Suruculeri: C:\\ (NTFS/VFS Sanal Disk) + FAT32 ESP Bolumu", CLR_TEXT_WHITE, 1);
}

/* ---- Windows 11 Aero Mouse İmleci --------------------------------- */

static void draw_win11_aero_cursor(int mx, int my)
{
    static const char *cursor_data[] = {
        "X               ",
        "XX              ",
        "X.X             ",
        "X..X            ",
        "X...X           ",
        "X....X          ",
        "X.....X         ",
        "X......X        ",
        "X.......X       ",
        "X........X      ",
        "X.....XXXXX     ",
        "X..X..X         ",
        "X.X X..X        ",
        "XX   X..X       ",
        "X     X..X      ",
        "       XX       "
    };

    /* Gölge */
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (cursor_data[y][x] != ' ') {
                fb_blend_rect(mx + x + 2, my + y + 2, 1, 1, 0x000000, 160);
            }
        }
    }

    /* Gövde */
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            char c = cursor_data[y][x];
            if (c == 'X') {
                fb_put_pixel(mx + x, my + y, 0x000000);
            } else if (c == '.') {
                fb_put_pixel(mx + x, my + y, 0xFFFFFF);
            }
        }
    }
}

/* ---- Genel GUI Arayüzü -------------------------------------------- */

void gui_init(void)
{
    g_w = fb_width();
    g_h = fb_height();
    if (g_w == 0 || g_h == 0) return;
}

void gui_redraw_all(void)
{
    if (!fb_ready()) return;

    /* 1. Fotogerçekçi Windows 11 Bloom Duvar Kağıdı */
    draw_photorealistic_bloom();

    /* 2. Masaüstü Simgeleri */
    draw_desktop_icons();

    /* 3. Windows 11 Sistem & Ayarlar Penceresi */
    int win_w = 780;
    int win_h = 440;
    int win_x = (int)(g_w / 2) - (win_w / 2);
    int win_y = 60;
    draw_win11_settings_window(win_x, win_y, win_w, win_h);

    /* 4. Windows 11 Başlat Menüsü */
    int center_x = (int)g_w / 2;
    int taskbar_y = (int)g_h - 50;
    draw_win11_start_menu(center_x, taskbar_y);

    /* 5. Ortalanmış Modern Görev Çubuğu */
    draw_win11_taskbar(1);

    /* 6. Windows 11 Aero Fare İmleci */
    draw_win11_aero_cursor(center_x + 60, taskbar_y - 120);
}

void gui_tick(int64_t boot_time, uint64_t uptime_seconds)
{
    if (!fb_ready()) return;

    g_boot_time = boot_time;
    g_uptime = uptime_seconds;

    char time_buf[16];
    ksnprintf(time_buf, sizeof(time_buf), "%02d:%02d",
              ((g_boot_time + g_uptime) / 3600) % 24,
              ((g_boot_time + g_uptime) / 60) % 60);

    int rx = (int)g_w - 90;
    int ty = (int)g_h - 50;

    fb_fill_rect(rx, ty + 8, 50, 16, 0x161616);
    fb_draw_text(rx, ty + 10, time_buf, CLR_TEXT_WHITE, 1);
}

void gui_panic(const char *message)
{
    fb_fill_rect(0, 0, (int)g_w, (int)g_h, 0x0078D4);
    fb_draw_text(80, 80, ":(", 0xFFFFFF, 8);
    fb_draw_text(80, 220, "Kisisel bilgisayariniz bir sorunla karsilasti ve yeniden baslatilmasi gerekiyor.", 0xFFFFFF, 2);
    fb_draw_text(80, 260, "Sadece bazi hata bilgileri toplaniyor, ardindan yeniden baslatilacak.", 0xFFFFFF, 1);
    fb_draw_text(80, 300, "%100 tamamlandi", 0xFFFFFF, 1);

    int qrx = 80, qry = 340;
    fb_fill_rect(qrx, qry, 64, 64, 0xFFFFFF);
    fb_fill_rect(qrx + 4, qry + 4, 20, 20, 0x0078D4);
    fb_fill_rect(qrx + 40, qry + 4, 20, 20, 0x0078D4);
    fb_fill_rect(qrx + 4, qry + 40, 20, 20, 0x0078D4);

    fb_draw_text(160, 350, "Durdurma Kodu: CRITICAL_PROCESS_DIED", 0xFFFFFF, 1);
    fb_draw_text(160, 370, "Basarisiz Olan: ntoskrnl.exe", 0xFFFFFF, 1);
    fb_draw_text(160, 390, message, 0xFFEB3B, 1);
}
