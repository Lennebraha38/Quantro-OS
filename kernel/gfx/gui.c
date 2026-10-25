/*
 * Quantro OS - Windows 11 Fluent Design & Mica Masaustu Grafik Motoru
 * Yuksek Cozunurluklu "Bloom" Duvar Kagidi, Ortalanmis Taskbar,
 * Baslat Menusu (Start Menu), Windows Ayarlar Penceresi ve Aero Mouse Cursor
 */
#include "gui.h"
#include "fb.h"
#include "kernel.h"
#include "sound.h"
#include "pmm.h"
#include "heap.h"

#include <stdint.h>

/* ---- Windows 11 Modern Renk Paleti (ARGB) ------------------------- */

#define RGB(r, g, b)        (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

#define C_BLOOM_DEEP        0x040914  /* Derin gece mavisi */
#define C_BLOOM_MID         0x0A1E38
#define C_BLOOM_BLUE        0x0F4C81
#define C_BLOOM_CYAN        0x1E88E5
#define C_BLOOM_LIGHT       0x64B5F6
#define C_BLOOM_HIGHLIGHT   0xBAE6FD
#define C_BLOOM_PURPLE      0x4F46E5

/* Windows 11 Dark Mode UI Elemanlari */
#define C_MICA_BG           0x202020  /* Windows 11 Koyu Gri Mica */
#define C_MICA_LIGHT        0x2B2B2B
#define C_MICA_CARD         0x272727
#define C_MICA_BORDER       0x383838
#define C_TASKBAR_BG        0x181818
#define C_TASKBAR_BORDER    0x303030

#define C_ACCENT_BLUE       0x0078D4  /* Windows 11 Brand Blue */
#define C_ACCENT_CYAN       0x60CDFF
#define C_TEXT_PRIMARY      0xFFFFFF
#define C_TEXT_SECONDARY    0x9E9E9E
#define C_TEXT_MUTED        0x757575

#define C_CLOSE_HOVER       0xC42B1C
#define C_SHADOW            0x000000

static uint32_t g_w = 1280;
static uint32_t g_h = 800;

static int64_t  g_boot_time = 0;
static uint64_t g_uptime = 0;

/* ---- Matematik ve Kavis Yardimcilari -------------------------------- */

static inline uint32_t blend_color(uint32_t src, uint32_t dst, uint8_t alpha)
{
    uint32_t sr = (src >> 16) & 0xFF, sg = (src >> 8) & 0xFF, sb = src & 0xFF;
    uint32_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
    uint32_t inv = 255 - alpha;
    uint32_t r = (sr * alpha + dr * inv) / 255;
    uint32_t g = (sg * alpha + dg * inv) / 255;
    uint32_t b = (sb * alpha + db * inv) / 255;
    return (r << 16) | (g << 8) | b;
}

/* ---- Windows 11 Ikon Cizicileri ----------------------------------- */

static void draw_win11_logo(int cx, int cy, int size)
{
    int half = size / 2;
    int gap = size > 24 ? 3 : 2;
    int q = half - gap;

    fb_fill_round_rect(cx - half, cy - half, q, q, 3, 0x00A4EF);
    fb_fill_round_rect(cx + gap,  cy - half, q, q, 3, 0x0078D4);
    fb_fill_round_rect(cx - half, cy + gap,  q, q, 3, 0x005A9E);
    fb_fill_round_rect(cx + gap,  cy + gap,  q, q, 3, 0x2899F5);
}

static void draw_edge_icon(int cx, int cy, int r)
{
    fb_fill_round_rect(cx - r, cy - r, r * 2, r * 2, r, 0x0C59A4);
    fb_fill_round_rect(cx - r + 4, cy - r + 4, r * 2 - 8, r * 2 - 8, r - 4, 0x1182C5);
    fb_fill_round_rect(cx - 2, cy - r + 6, r + 2, r, r / 2, 0x00EBFF);
    fb_draw_text(cx - 4, cy - 6, "e", 0xFFFFFF, 2);
}

static void draw_explorer_icon(int cx, int cy, int size)
{
    int w = size;
    int h = size * 3 / 4;
    int x = cx - w / 2;
    int y = cy - h / 2;

    /* Arka sekme */
    fb_fill_round_rect(x, y - 4, w / 2, 8, 3, 0x1A73E8);
    /* Mavi klasör arka planı */
    fb_fill_round_rect(x, y, w, h, 4, 0x2196F3);
    /* Ön sarı cep */
    fb_fill_round_rect(x + 2, y + 4, w - 4, h - 6, 3, 0xFFC107);
    fb_fill_round_rect(x + 2, y + 8, w - 4, h - 10, 3, 0xFFD54F);
}

static void draw_terminal_icon(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, 5, 0x0C0C0C);
    fb_fill_round_rect(cx - r + 1, cy - r + 1, size - 2, size - 2, 4, 0x1C1C1C);
    /* Renkli tab cubugu */
    fb_fill_rect(cx - r + 4, cy - r + 3, size - 8, 2, 0x4CC2FF);
    fb_draw_text(cx - 9, cy - 2, ">_", 0xFFFFFF, 1);
}

static void draw_settings_icon(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, 6, 0x303030);
    /* Disli disleri */
    fb_fill_round_rect(cx - 5, cy - 5, 10, 10, 2, 0x0078D4);
    fb_fill_round_rect(cx - 2, cy - 2, 4, 4, 1, 0xFFFFFF);
}

static void draw_store_icon(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r + 4, size, size - 6, 4, 0x2A2A2A);
    /* Canta sapı */
    fb_fill_round_rect(cx - r / 2, cy - r, r, 6, 3, 0x0078D4);
    /* Windows 4 renkli karesi */
    draw_win11_logo(cx, cy + 2, 10);
}

static void draw_calc_icon(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, 5, 0x262626);
    fb_fill_round_rect(cx - r + 3, cy - r + 3, size - 6, 7, 2, 0x383838);
    fb_draw_text(cx - 7, cy, "+ -", 0x4CC2FF, 1);
}

/* ---- Gerçek Windows 11 "Bloom" Duvar Kağıdı Motoru ---------------- */

static void draw_bloom_wallpaper(void)
{
    /* 1. Gece Mavisi Arka Plan Dikey Gradyanı */
    for (uint32_t y = 0; y < g_h; y++) {
        uint32_t t = (y * 255) / (g_h ? g_h : 1);
        uint32_t r = 3 + (t * 7) / 255;
        uint32_t g = 9 + (t * 22) / 255;
        uint32_t b = 20 + (t * 40) / 255;
        fb_fill_rect(0, (int)y, (int)g_w, 1, RGB(r, g, b));
    }

    /* 2. Merkez Bloom Işık Odakları (Volumetric Glow) */
    int cx = (int)g_w / 2;
    int cy = (int)g_h / 2 - 20;

    /* Buyuk arka mavi isik halkasi */
    for (int radius = 320; radius > 20; radius -= 8) {
        uint8_t alpha = (uint8_t)((320 - radius) * 65 / 320);
        fb_blend_round_rect(cx - radius, cy - radius * 3 / 4,
                            radius * 2, radius * 3 / 2, radius / 2,
                            C_BLOOM_BLUE, alpha);
    }

    /* 3. Bloom Petal (İpeksi Kıvrımlar / Katmanlar) */
    /* Sol Katman (Indigo/Cyan Petal) */
    for (int i = 0; i < 40; i++) {
        int px = cx - 180 + i * 4;
        int py = cy + 40 - i * 6;
        int pw = 120 + i * 2;
        int ph = 180 - i * 2;
        fb_blend_round_rect(px, py, pw, ph, 45, C_BLOOM_CYAN, 8);
    }

    /* Sag Katman (Mor/Mavi Petal) */
    for (int i = 0; i < 45; i++) {
        int px = cx - 20 + i * 5;
        int py = cy + 60 - i * 5;
        int pw = 140 + i;
        int ph = 190 - i * 2;
        fb_blend_round_rect(px, py, pw, ph, 55, C_BLOOM_PURPLE, 7);
    }

    /* Merkez Parlama (Cyan/White Core) */
    for (int r = 140; r > 10; r -= 6) {
        uint8_t alpha = (uint8_t)((140 - r) * 90 / 140);
        fb_blend_round_rect(cx - r * 3 / 4, cy - r / 2, r * 3 / 2, r, r / 2,
                            C_BLOOM_HIGHLIGHT, alpha);
    }
}

/* ---- Masaüstü Simgeleri (Desktop Icons) ---------------------------- */

static void draw_desktop_icon(int x, int y, const char *label, int type)
{
    /* Simge Arka Plan Hover Efekti */
    fb_fill_round_rect(x - 4, y - 4, 76, 76, 6, 0x000000);
    fb_blend_round_rect(x - 4, y - 4, 76, 76, 6, 0xFFFFFF, 12);

    int icx = x + 34;
    int icy = y + 24;

    if (type == 0) {
        /* Bu Bilgisayar */
        fb_fill_round_rect(icx - 16, icy - 12, 32, 22, 3, 0x37474F);
        fb_fill_rect(icx - 13, icy - 9, 26, 16, 0x81D4FA);
        fb_fill_rect(icx - 4, icy + 10, 8, 4, 0xB0BEC5);
        fb_fill_rect(icx - 8, icy + 14, 16, 2, 0xB0BEC5);
    } else if (type == 1) {
        /* Geri Donusum Kutusu */
        fb_fill_round_rect(icx - 12, icy - 14, 24, 28, 4, 0x455A64);
        fb_fill_rect(icx - 8, icy - 10, 16, 22, 0x90A4AE);
        fb_draw_text(icx - 5, icy - 4, "♻", 0xFFFFFF, 1);
    } else if (type == 2) {
        /* Microsoft Edge */
        draw_edge_icon(icx, icy, 16);
    } else {
        /* Quantro Terminal */
        draw_terminal_icon(icx, icy, 32);
    }

    /* Etiket */
    fb_draw_text_centered(icx, y + 54, label, C_TEXT_PRIMARY, 1);
}

/* ---- Windows 11 Başlat Menüsü (Start Menu Popup) ------------------- */

static void draw_start_menu(int cx, int bottom_y)
{
    int mw = 560;
    int mh = 500;
    int mx = cx - mw / 2;
    int my = bottom_y - mh - 12;

    /* 1. Geniş ve Yumuşak Drop Shadow (Pencere Gölgesi) */
    fb_blend_round_rect(mx - 8, my - 6, mw + 16, mh + 16, 18, C_SHADOW, 160);
    fb_blend_round_rect(mx - 4, my - 3, mw + 8, mh + 8, 16, C_SHADOW, 180);

    /* 2. Mica Dark Arka Planı */
    fb_fill_round_rect(mx, my, mw, mh, 12, C_MICA_BG);
    fb_blend_round_rect(mx, my, mw, mh, 12, 0x2A2A2A, 180);

    /* 1px Şık Kenarlık */
    fb_fill_round_rect(mx, my, mw, 1, 1, C_MICA_BORDER);
    fb_fill_round_rect(mx, my, 1, mh, 1, C_MICA_BORDER);
    fb_fill_round_rect(mx + mw - 1, my, 1, mh, 1, C_MICA_BORDER);

    /* 3. Arama Kutusu (Search Bar) */
    int sx = mx + 32;
    int sy = my + 28;
    int sw = mw - 64;
    int sh = 36;
    fb_fill_round_rect(sx, sy, sw, sh, 18, 0x1E1E1E);
    fb_fill_round_rect(sx, sy, sw, 1, 1, 0x3A3A3A);
    fb_draw_text(sx + 16, sy + 10, "🔍 Aramak icin buraya yazin...", C_TEXT_MUTED, 1);

    /* 4. Sabitlenenler (Pinned) Başlığı */
    int py = sy + 50;
    fb_draw_text(mx + 36, py, "Sabitlenenler", C_TEXT_PRIMARY, 1);
    fb_draw_text(mx + mw - 150, py, "Tum uygulamalar >", C_ACCENT_CYAN, 1);

    /* 5. Sabitlenen Uygulamalar Izgarasi (3 Satir x 6 Sutun) */
    int app_x_start = mx + 46;
    int app_y_start = py + 24;
    int col_step = (mw - 92) / 6;

    /* 1. Satir */
    draw_edge_icon(app_x_start + 0 * col_step, app_y_start, 16);
    fb_draw_text_centered(app_x_start + 0 * col_step, app_y_start + 20, "Edge", C_TEXT_PRIMARY, 1);

    draw_explorer_icon(app_x_start + 1 * col_step, app_y_start, 30);
    fb_draw_text_centered(app_x_start + 1 * col_step, app_y_start + 20, "Gezgin", C_TEXT_PRIMARY, 1);

    draw_store_icon(app_x_start + 2 * col_step, app_y_start, 28);
    fb_draw_text_centered(app_x_start + 2 * col_step, app_y_start + 20, "Store", C_TEXT_PRIMARY, 1);

    draw_settings_icon(app_x_start + 3 * col_step, app_y_start, 28);
    fb_draw_text_centered(app_x_start + 3 * col_step, app_y_start + 20, "Ayarlar", C_TEXT_PRIMARY, 1);

    draw_calc_icon(app_x_start + 4 * col_step, app_y_start, 28);
    fb_draw_text_centered(app_x_start + 4 * col_step, app_y_start + 20, "Hesap", C_TEXT_PRIMARY, 1);

    draw_terminal_icon(app_x_start + 5 * col_step, app_y_start, 28);
    fb_draw_text_centered(app_x_start + 5 * col_step, app_y_start + 20, "Terminal", C_TEXT_PRIMARY, 1);

    /* 2. Satir */
    int row2_y = app_y_start + 54;
    fb_fill_round_rect(app_x_start + 0 * col_step - 14, row2_y - 14, 28, 28, 5, 0x185ABD);
    fb_draw_text_centered(app_x_start + 0 * col_step, row2_y - 5, "W", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x_start + 0 * col_step, row2_y + 18, "Word", C_TEXT_PRIMARY, 1);

    fb_fill_round_rect(app_x_start + 1 * col_step - 14, row2_y - 14, 28, 28, 5, 0x107C41);
    fb_draw_text_centered(app_x_start + 1 * col_step, row2_y - 5, "X", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x_start + 1 * col_step, row2_y + 18, "Excel", C_TEXT_PRIMARY, 1);

    fb_fill_round_rect(app_x_start + 2 * col_step - 14, row2_y - 14, 28, 28, 5, 0xC43E1C);
    fb_draw_text_centered(app_x_start + 2 * col_step, row2_y - 5, "P", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x_start + 2 * col_step, row2_y + 18, "P.Point", C_TEXT_PRIMARY, 1);

    fb_fill_round_rect(app_x_start + 3 * col_step - 14, row2_y - 14, 28, 28, 5, 0x0078D4);
    fb_draw_text_centered(app_x_start + 3 * col_step, row2_y - 5, "✉", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x_start + 3 * col_step, row2_y + 18, "Posta", C_TEXT_PRIMARY, 1);

    fb_fill_round_rect(app_x_start + 4 * col_step - 14, row2_y - 14, 28, 28, 5, 0xE74C3C);
    fb_draw_text_centered(app_x_start + 4 * col_step, row2_y - 5, "📅", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x_start + 4 * col_step, row2_y + 18, "Takvim", C_TEXT_PRIMARY, 1);

    fb_fill_round_rect(app_x_start + 5 * col_step - 14, row2_y - 14, 28, 28, 5, 0x2C3E50);
    fb_draw_text_centered(app_x_start + 5 * col_step, row2_y - 5, "📝", 0xFFFFFF, 1);
    fb_draw_text_centered(app_x_start + 5 * col_step, row2_y + 18, "Notlar", C_TEXT_PRIMARY, 1);

    /* 6. Önerilenler (Recommended) */
    int rec_y = row2_y + 54;
    fb_draw_text(mx + 36, rec_y, "Onerilenler", C_TEXT_PRIMARY, 1);

    /* Oneri Kart 1 */
    int card1_y = rec_y + 20;
    draw_explorer_icon(mx + 54, card1_y + 10, 20);
    fb_draw_text(mx + 76, card1_y + 4, "welcome.txt", C_TEXT_PRIMARY, 1);
    fb_draw_text(mx + 76, card1_y + 18, "C:\\Users\\Quantro\\Desktop", C_TEXT_MUTED, 1);

    /* Oneri Kart 2 */
    int card2_x = mx + mw / 2 + 10;
    fb_fill_round_rect(card2_x, card1_y + 2, 20, 20, 4, 0x0078D4);
    fb_draw_text(card2_x + 5, card1_y + 6, "NT", 0xFFFFFF, 1);
    fb_draw_text(card2_x + 30, card1_y + 4, "ntoskrnl.exe", C_TEXT_PRIMARY, 1);
    fb_draw_text(card2_x + 30, card1_y + 18, "C:\\Windows\\System32", C_TEXT_MUTED, 1);

    /* 7. Alt Kullanıcı ve Güç Çubuğu (Footer) */
    int foot_y = my + mh - 58;
    fb_fill_round_rect(mx, foot_y, mw, 58, 12, 0x1A1A1A);
    fb_fill_rect(mx, foot_y, mw, 1, C_MICA_BORDER);

    /* Profil Resmi */
    fb_fill_round_rect(mx + 32, foot_y + 14, 30, 30, 15, C_ACCENT_BLUE);
    fb_draw_text(mx + 42, foot_y + 20, "Q", 0xFFFFFF, 1);
    fb_draw_text(mx + 72, foot_y + 18, "QuantroUser (Yonetici)", C_TEXT_PRIMARY, 1);
    fb_draw_text(mx + 72, foot_y + 32, "quantro@live.com", C_TEXT_MUTED, 1);

    /* Kapat / Guc Butonu */
    int pwr_x = mx + mw - 56;
    fb_fill_round_rect(pwr_x, foot_y + 14, 32, 32, 6, 0x282828);
    fb_draw_text(pwr_x + 10, foot_y + 22, "⏻", 0xFF5555, 1);
}

/* ---- Modern Windows 11 Görev Çubuğu (Taskbar) ---------------------- */

static void draw_modern_taskbar(int start_menu_open)
{
    int th = 50;
    int ty = (int)g_h - th;

    /* Taskbar Arka Plan (Mica Dark Blur) */
    fb_fill_rect(0, ty, (int)g_w, th, C_TASKBAR_BG);
    fb_fill_rect(0, ty, (int)g_w, 1, C_TASKBAR_BORDER);

    /* Sol Taraf: Hava Durumu Widget'i */
    fb_fill_round_rect(16, ty + 8, 140, 34, 6, 0x222222);
    fb_draw_text(26, ty + 16, "☀ 21 C Bulutlu", C_TEXT_PRIMARY, 1);

    /* Ortalanmış Uygulama Simgeleri */
    int icon_count = 7;
    int icon_box = 42;
    int center_start = (int)(g_w / 2) - ((icon_count * icon_box) / 2);
    int cur_x = center_start;

    /* 1. Windows Başlat Butonu */
    if (start_menu_open) {
        fb_fill_round_rect(cur_x - 2, ty + 4, icon_box, icon_box, 6, 0x333333);
        /* Butonun altinda kucuk aktif cizgisi */
        fb_fill_round_rect(cur_x + 12, ty + th - 4, 14, 3, 1, C_ACCENT_BLUE);
    }
    draw_win11_logo(cur_x + icon_box / 2, ty + th / 2, 24);
    cur_x += icon_box;

    /* 2. Edge */
    draw_edge_icon(cur_x + icon_box / 2, ty + th / 2, 14);
    fb_fill_round_rect(cur_x + 16, ty + th - 4, 6, 3, 1, C_TEXT_MUTED);
    cur_x += icon_box;

    /* 3. Dosya Gezgini */
    draw_explorer_icon(cur_x + icon_box / 2, ty + th / 2, 28);
    fb_fill_round_rect(cur_x + 12, ty + th - 4, 14, 3, 1, C_ACCENT_BLUE); /* Acik pencere */
    cur_x += icon_box;

    /* 4. Microsoft Store */
    draw_store_icon(cur_x + icon_box / 2, ty + th / 2, 26);
    cur_x += icon_box;

    /* 5. Terminal */
    draw_terminal_icon(cur_x + icon_box / 2, ty + th / 2, 26);
    cur_x += icon_box;

    /* 6. Hesap Makinesi */
    draw_calc_icon(cur_x + icon_box / 2, ty + th / 2, 26);
    cur_x += icon_box;

    /* 7. Ayarlar */
    draw_settings_icon(cur_x + icon_box / 2, ty + th / 2, 26);
    fb_fill_round_rect(cur_x + 12, ty + th - 4, 14, 3, 1, C_ACCENT_BLUE);
    cur_x += icon_box;

    /* Sağ Taraf: Sistem Tepsisi (Tray) */
    int rx = (int)g_w - 24;

    /* Saat ve Tarih Kutusu */
    char time_buf[16], date_buf[16];
    ksnprintf(time_buf, sizeof(time_buf), "%02d:%02d",
              ((g_boot_time + g_uptime) / 3600) % 24,
              ((g_boot_time + g_uptime) / 60) % 60);
    ksnprintf(date_buf, sizeof(date_buf), "25.10.2026");

    uint32_t tw = fb_text_width(date_buf, 1);
    rx -= (int)tw + 12;

    fb_draw_text(rx + 16, ty + 10, time_buf, C_TEXT_PRIMARY, 1);
    fb_draw_text(rx, ty + 26, date_buf, C_TEXT_MUTED, 1);

    /* Durum Simgeleri (Hap Şeklinde Tepsi Kutusu) */
    rx -= 100;
    fb_fill_round_rect(rx, ty + 8, 92, 34, 17, 0x262626);
    fb_draw_text(rx + 12, ty + 16, "📶 🔊 🔋", C_TEXT_PRIMARY, 1);
}

/* ---- Gerçek Windows 11 Ayarlar & Sistem Penceresi ------------------ */

static void draw_settings_window(int wx, int wy, int ww, int wh)
{
    /* 1. Pencere Drop Shadow */
    fb_blend_round_rect(wx - 8, wy - 6, ww + 16, wh + 16, 16, C_SHADOW, 170);

    /* 2. Gövde */
    fb_fill_round_rect(wx, wy, ww, wh, 10, C_MICA_BG);
    fb_fill_round_rect(wx, wy, ww, 1, 1, C_MICA_BORDER);

    /* 3. Başlık Çubuğu */
    fb_fill_round_rect(wx, wy, ww, 40, 10, 0x262626);
    fb_fill_rect(wx, wy + 30, ww, 10, 0x262626);

    /* Başlık Simgesi ve Metin */
    draw_settings_icon(wx + 20, wy + 20, 20);
    fb_draw_text(wx + 44, wy + 14, "Ayarlar - Sistem Ozellikleri", C_TEXT_PRIMARY, 1);

    /* Pencere Kontrolleri (Sağ Üst) */
    int cx_btn = wx + ww - 46;
    /* Kapat */
    fb_fill_round_rect(cx_btn, wy + 6, 38, 28, 4, 0x262626);
    fb_draw_text(cx_btn + 14, wy + 12, "✕", C_TEXT_PRIMARY, 1);

    /* Büyüt */
    cx_btn -= 42;
    fb_fill_round_rect(cx_btn, wy + 6, 38, 28, 4, 0x262626);
    fb_draw_text(cx_btn + 13, wy + 12, "□", C_TEXT_PRIMARY, 1);

    /* Küçült */
    cx_btn -= 42;
    fb_fill_round_rect(cx_btn, wy + 6, 38, 28, 4, 0x262626);
    fb_draw_text(cx_btn + 13, wy + 12, "—", C_TEXT_PRIMARY, 1);

    /* 4. Sol Yan Menü (Sidebar) */
    int sb_w = 180;
    int sb_h = wh - 40;
    int sb_x = wx;
    int sb_y = wy + 40;
    fb_fill_rect(sb_x, sb_y, sb_w, sb_h, 0x232323);
    fb_fill_rect(sb_x + sb_w - 1, sb_y, 1, sb_h, C_MICA_BORDER);

    /* Sidebar Elemanlari */
    int my = sb_y + 16;
    /* 1. Sistem (Aktif) */
    fb_fill_round_rect(sb_x + 10, my - 4, sb_w - 20, 32, 6, 0x333333);
    fb_fill_round_rect(sb_x + 10, my + 4, 3, 16, 1, C_ACCENT_BLUE);
    fb_draw_text(sb_x + 24, my + 4, "🖥 Sistem", C_TEXT_PRIMARY, 1);
    my += 36;

    fb_draw_text(sb_x + 24, my + 4, "📶 Ag & Internet", C_TEXT_SECONDARY, 1);
    my += 34;

    fb_draw_text(sb_x + 24, my + 4, "🎨 Kisisellestir", C_TEXT_SECONDARY, 1);
    my += 34;

    fb_draw_text(sb_x + 24, my + 4, "🔒 Guvenlik & TPM", C_TEXT_SECONDARY, 1);
    my += 34;

    fb_draw_text(sb_x + 24, my + 4, "⚙ Windows Update", C_TEXT_SECONDARY, 1);

    /* 5. Sağ İçerik Alanı */
    int cx = sb_x + sb_w + 24;
    int cy = sb_y + 16;

    /* Cihaz Başlığı ve PC Resmi */
    draw_win11_logo(cx + 24, cy + 24, 48);
    fb_draw_text(cx + 64, cy + 12, "QUANTRO-PC (ARM64 Virt)", C_TEXT_PRIMARY, 2);
    fb_draw_text(cx + 64, cy + 34, "Windows 11 Pro 24H2 | OS Derlemesi 26100.1742", C_TEXT_MUTED, 1);
    cy += 70;

    /* Kart 1: Cihaz Özellikleri */
    int cw = ww - sb_w - 48;
    fb_fill_round_rect(cx, cy, cw, 78, 8, C_MICA_CARD);
    fb_fill_round_rect(cx, cy, cw, 1, 1, C_MICA_BORDER);
    fb_draw_text(cx + 16, cy + 10, "Cihaz Ozellikleri", C_ACCENT_CYAN, 1);
    fb_draw_text(cx + 16, cy + 30, "Islemci:       ARM Cortex-A53 @ 1.50 GHz (4 vCPU AArch64)", C_TEXT_PRIMARY, 1);
    fb_draw_text(cx + 16, cy + 46, "Bellek:        1024 MB RAM (PMM 4KB Granule + 2MB Kernel Heap)", C_TEXT_PRIMARY, 1);
    fb_draw_text(cx + 16, cy + 62, "Sistem Turu:   64-bit Isletim Sistemi, ARM tabanli islemci", C_TEXT_SECONDARY, 1);
    cy += 88;

    /* Kart 2: Windows 11 Donanım Güvenliği */
    fb_fill_round_rect(cx, cy, cw, 78, 8, C_MICA_CARD);
    fb_fill_round_rect(cx, cy, cw, 1, 1, C_MICA_BORDER);
    fb_draw_text(cx + 16, cy + 10, "Donanim & Guvenlik Standartlari", 0x66BB6A, 1);
    fb_draw_text(cx + 16, cy + 30, "[✔] TPM 2.0:        Etkin & Kilitli (PCR0-7 Donanim Dogrulamasi)", C_TEXT_PRIMARY, 1);
    fb_draw_text(cx + 16, cy + 46, "[✔] Secure Boot:    UEFI Platform Key (PK) Onayli", C_TEXT_PRIMARY, 1);
    fb_draw_text(cx + 16, cy + 62, "[✔] Bellek Korumasi: VBS + HVCI (W^X Enforced)", C_TEXT_SECONDARY, 1);
    cy += 88;

    /* Kart 3: Ağ & Depolama */
    fb_fill_round_rect(cx, cy, cw, 68, 8, C_MICA_CARD);
    fb_fill_round_rect(cx, cy, cw, 1, 1, C_MICA_BORDER);
    fb_draw_text(cx + 16, cy + 10, "Ag & Depolama Durumu", 0xFFA726, 1);
    fb_draw_text(cx + 16, cy + 30, "NDIS Baglantisi: 192.168.1.105 (VirtIO 10Gbps Ethernet0 - Link UP)", C_TEXT_PRIMARY, 1);
    fb_draw_text(cx + 16, cy + 48, "Disk Suruculeri: C:\\ (NTFS/VFS Sanal Disk) + FAT32 ESP Bolumu", C_TEXT_PRIMARY, 1);
}

/* ---- Gerçek Windows 11 Aero Mouse İmleci -------------------------- */

static void draw_aero_cursor(int mx, int my)
{
    /* 12x18 Windows 11 Beyaz Ok İmleci (Sprite Haritası) */
    static const char *cursor_map[] = {
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

    /* İmleç Gölgesi */
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (cursor_map[y][x] != ' ') {
                fb_blend_rect(mx + x + 2, my + y + 2, 1, 1, 0x000000, 160);
            }
        }
    }

    /* İmleç Gövdesi */
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            char c = cursor_map[y][x];
            if (c == 'X') {
                /* Siyah kenarlık */
                fb_put_pixel(mx + x, my + y, 0x000000);
            } else if (c == '.') {
                /* Parlak beyaz gövde */
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

    /* 1. Windows 11 İkonik Bloom Duvar Kağıdı */
    draw_bloom_wallpaper();

    /* 2. Masaüstü Simgeleri */
    draw_desktop_icon(24, 24, "Bu Bilgisayar", 0);
    draw_desktop_icon(24, 114, "Geri Donusum", 1);
    draw_desktop_icon(24, 204, "MS Edge", 2);
    draw_desktop_icon(24, 294, "Terminal", 3);

    /* 3. Açık Windows 11 Ayarlar / Sistem Penceresi */
    int win_w = 780;
    int win_h = 440;
    int win_x = (int)(g_w / 2) - (win_w / 2);
    int win_y = 60;
    draw_settings_window(win_x, win_y, win_w, win_h);

    /* 4. Windows 11 Başlat Menüsü (Start Menu Popup) */
    int center_x = (int)g_w / 2;
    int taskbar_y = (int)g_h - 50;
    draw_start_menu(center_x, taskbar_y);

    /* 5. Ortalanmış Modern Görev Çubuğu (Taskbar) */
    draw_modern_taskbar(1); /* Başlat açık */

    /* 6. Windows 11 Aero Fare İmleci */
    draw_aero_cursor(center_x + 60, taskbar_y - 120);
}

void gui_tick(int64_t boot_time, uint64_t uptime_seconds)
{
    if (!fb_ready()) return;

    g_boot_time = boot_time;
    g_uptime = uptime_seconds;

    /* Saati saniyede bir canli yenile */
    char time_buf[16];
    ksnprintf(time_buf, sizeof(time_buf), "%02d:%02d",
              ((g_boot_time + g_uptime) / 3600) % 24,
              ((g_boot_time + g_uptime) / 60) % 60);

    int rx = (int)g_w - 90;
    int ty = (int)g_h - 50;

    fb_fill_rect(rx, ty + 8, 50, 16, C_TASKBAR_BG);
    fb_draw_text(rx, ty + 10, time_buf, C_TEXT_PRIMARY, 1);
}

void gui_panic(const char *message)
{
    /* Windows 11 Gerçek Mavi Ekran (BSOD) */
    fb_fill_rect(0, 0, (int)g_w, (int)g_h, 0x0078D4);
    fb_draw_text(80, 80, ":(", 0xFFFFFF, 8);
    fb_draw_text(80, 220, "Kisisel bilgisayariniz bir sorunla karsilasti ve yeniden baslatilmasi gerekiyor.", 0xFFFFFF, 2);
    fb_draw_text(80, 260, "Sadece bazi hata bilgileri toplaniyor, ardindan yeniden baslatilacak.", 0xFFFFFF, 1);
    fb_draw_text(80, 300, "%100 tamamlandi", 0xFFFFFF, 1);

    /* QR Kod Simülasyonu */
    int qrx = 80, qry = 340;
    fb_fill_rect(qrx, qry, 64, 64, 0xFFFFFF);
    fb_fill_rect(qrx + 4, qry + 4, 20, 20, 0x0078D4);
    fb_fill_rect(qrx + 40, qry + 4, 20, 20, 0x0078D4);
    fb_fill_rect(qrx + 4, qry + 40, 20, 20, 0x0078D4);

    fb_draw_text(160, 350, "Durdurma Kodu: CRITICAL_PROCESS_DIED", 0xFFFFFF, 1);
    fb_draw_text(160, 370, "Basarisiz Olan: ntoskrnl.exe", 0xFFFFFF, 1);
    fb_draw_text(160, 390, message, 0xFFEB3B, 1);
}
