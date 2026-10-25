/*
 * Quantro OS - Ultra Realistic Windows 11 Fluent & Mica GUI Motoru
 * Vizyoner Açılış Ekranı, Keskin Tipografi, Çoklu İnteraktif Uygulamalar ve Pencere Yöneticisi
 */
#include "gui.h"
#include "fb.h"
#include "kernel.h"
#include "sound.h"
#include "types.h"

#define RGB(r, g, b)        (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

/* Windows 11 Fluent & Cosmic Renk Paleti */
#define CLR_BLOOM_PETAL1    0x0078D4
#define CLR_BLOOM_PETAL2    0x00A4EF
#define CLR_BLOOM_PETAL3    0x2899F5
#define CLR_BLOOM_PETAL4    0x60CDFF
#define CLR_BLOOM_GLOW      0xBAE6FD
#define CLR_BLOOM_PURPLE    0x5B21B6

#define CLR_MICA_BG         0x202020
#define CLR_MICA_CARD       0x2B2B2B
#define CLR_TEXT_WHITE      0xFFFFFF
#define CLR_TEXT_GRAY       0xCCCCCC
#define CLR_TEXT_MUTED      0x888888
#define CLR_ACCENT_BLUE     0x0078D4
#define CLR_ACCENT_CYAN     0x60CDFF
#define CLR_ACCENT_PURPLE   0x8B5CF6
#define CLR_ACCENT_GREEN    0x10B981

static uint32_t g_w = 1280;
static uint32_t g_h = 800;

static int64_t  g_boot_time = 0;
static uint64_t g_uptime = 0;

/* Etkileşimli Durumlar */
static int g_cursor_x = 640;
static int g_cursor_y = 400;
static int g_start_open = 0;

/* Çoklu Uygulama ve Pencere Yönetimi */
typedef struct {
    int open;
    int minimized;
    int maximized;
    int x, y, w, h;
    int orig_x, orig_y, orig_w, orig_h;
} win_state_t;

static int g_active_app = APP_SETTINGS;
static win_state_t g_windows[8];
static int g_quick_settings_open = 0;
static int g_notifications_open = 0;
static int g_snap_popup_open = 0;

/* Uygulama Özel Durumları */
/* 1. Ayarlar */
static int g_settings_tab = 0; /* 0: Sistem, 1: Ag, 2: Kisisellestirme, 3: Guvenlik */
static int g_theme_accent = 0; /* 0: Mavi, 1: Mor, 2: Yesil, 3: Cyan */

/* 2. Dosya Gezgini */
static int g_explorer_path = 0; /* 0: C:\, 1: C:\Windows, 2: C:\Windows\System32, 3: C:\Users\Quantro */
static int g_explorer_selected = -1;

/* 3. Hesap Makinesi */
static char g_calc_display[32] = "0";
static int  g_calc_accum = 0;
static char g_calc_op = 0;
static int  g_calc_fresh = 1;

/* 4. Terminal */
static char g_term_lines[12][80] = {
    "Microsoft Windows [Surum 10.0.26100.1742]",
    "(c) Microsoft Corporation. Tum haklari saklidir.",
    "",
    "PS C:\\Users\\Quantro> uname -a",
    "QuantroOS 2.0 AArch64 ARM-v8A Limine-Rev2 SMP",
    "PS C:\\Users\\Quantro> "
};
static int g_term_line_count = 6;

/* 5. Edge */
static int g_edge_tab = 0; /* 0: Quantro Hub, 1: GitHub, 2: Wikipedia */

/* İmleç Arka Plan Önbelleği (Sıfır Gecikme) */
static uint32_t s_cursor_bg[24][24];
static int s_saved_cx = -1;
static int s_saved_cy = -1;
static int s_has_saved_bg = 0;

/* ---- İkon Çizicileri ---------------------------------------------- */

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
    fb_draw_text(cx - 4, cy - 8, "e", 0xFFFFFF, 1);
}

static void draw_fluent_explorer(int cx, int cy, int size)
{
    int w = size;
    int h = size * 3 / 4;
    int x = cx - w / 2;
    int y = cy - h / 2;

    fb_fill_round_rect(x, y - 4, w / 2, 8, 3, 0x005FB8);
    fb_fill_round_rect(x, y, w, h, 4, 0x0078D4);
    fb_fill_round_rect(x + 2, y + 4, w - 4, h - 6, 3, 0xFFA000);
    fb_fill_round_rect(x + 2, y + 8, w - 4, h - 10, 3, 0xFFC107);
}

static void draw_fluent_terminal(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, 5, 0x111111);
    fb_fill_round_rect(cx - r + 1, cy - r + 1, size - 2, size - 2, 4, 0x1E1E1E);
    fb_fill_rect(cx - r + 4, cy - r + 3, size - 8, 2, 0x60CDFF);
    fb_draw_text(cx - 7, cy - 7, ">_", 0xFFFFFF, 1);
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
    fb_fill_round_rect(cx - r, cy - r, size, size, 6, 0x0078D4);
    fb_draw_round_border(cx - 6, cy - 8, 12, 8, 4, 2, 0xFFFFFF);
    draw_win_logo(cx, cy + 2, 12);
}

static void draw_fluent_calc(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, 6, 0x242424);
    fb_fill_round_rect(cx - r + 3, cy - r + 3, size - 6, size / 3, 2, 0x333333);
    fb_draw_text(cx - 7, cy - 3, "+-", 0x60CDFF, 1);
}

static void draw_fluent_taskmgr(int cx, int cy, int size)
{
    int r = size / 2;
    fb_fill_round_rect(cx - r, cy - r, size, size, 6, 0x1E1E1E);
    fb_fill_round_rect(cx - r + 4, cy + 2, 4, 8, 1, 0x10B981);
    fb_fill_round_rect(cx - r + 10, cy - 4, 4, 14, 1, 0x0078D4);
    fb_fill_round_rect(cx - r + 16, cy - 8, 4, 18, 1, 0xF59E0B);
}

/* ---- Vizyoner & Zamansız Açılış Ekranı (Visionary Startup) -------- */

void gui_show_visionary_boot(void)
{
    if (!fb_ready()) return;

    uint32_t w = fb_width();
    uint32_t h = fb_height();
    int cx = (int)w / 2;
    int cy = (int)h / 2 - 30;

    /* 1. Derin Kozmik Obsidyen Arka Plan */
    for (uint32_t y = 0; y < h; y++) {
        uint32_t t = (y * 255) / (h ? h : 1);
        uint32_t r = 2 + (t * 6) / 255;
        uint32_t g = 4 + (t * 10) / 255;
        uint32_t b = 12 + (t * 22) / 255;
        fb_fill_rect(0, (int)y, (int)w, 1, RGB(r, g, b));
    }

    /* 2. Kozmik Kuantum Yıldız Tozu */
    for (int i = 0; i < 60; i++) {
        int sx = (int)((uint64_t)(i * 997 + 101) % w);
        int sy = (int)((uint64_t)(i * 733 + 53) % h);
        uint8_t bright = (uint8_t)(80 + (i * 17) % 175);
        fb_fill_rect(sx, sy, 2, 2, RGB(bright, bright, bright + 20));
    }

    /* 3. Luminous Quantum Core (Merkez Işıltılı Kuantum Rozeti) */
    for (int r = 160; r > 20; r -= 6) {
        uint8_t a = (uint8_t)((160 - r) * 45 / 160);
        fb_blend_round_rect(cx - r, cy - r, r * 2, r * 2, r, CLR_BLOOM_PETAL1, a);
    }
    for (int r = 100; r > 10; r -= 5) {
        uint8_t a = (uint8_t)((100 - r) * 75 / 100);
        fb_blend_round_rect(cx - r, cy - r, r * 2, r * 2, r, CLR_BLOOM_PETAL4, a);
    }

    /* Kuantum Çekirdek Elmas Kristali */
    fb_fill_round_rect(cx - 36, cy - 36, 72, 72, 16, 0x0078D4);
    fb_fill_round_rect(cx - 26, cy - 26, 52, 52, 12, 0x00A4EF);
    fb_fill_round_rect(cx - 16, cy - 16, 32, 32, 8,  0x60CDFF);
    fb_fill_round_rect(cx - 6,  cy - 6,  12, 12, 4,  0xFFFFFF);

    /* 4. Keskin Tipografi: QUANTRO */
    fb_draw_text_centered(cx, cy + 70, "Q U A N T R O   O S", 0xFFFFFF, 2);
    fb_draw_text_centered(cx, cy + 105, "N E X T - G E N E R A T I O N   N E U R A L   A R C H I T E C T U R E", 0x60CDFF, 1);

    /* 5. Holografik İlerleme Çubuğu ve Telemetri */
    int bar_w = 420;
    int bar_h = 8;
    int bar_x = cx - bar_w / 2;
    int bar_y = cy + 140;

    fb_fill_round_rect(bar_x, bar_y, bar_w, bar_h, 4, 0x1E293B);
    fb_blend_round_rect(bar_x, bar_y, bar_w, 1, 1, 0xFFFFFF, 40);

    const char *stages[4] = {
        ">> AArch64 4-Level MMU & PMM Sayfa Tablolari Aktif...",
        ">> TPM 2.0 PCR0-7 Donanim Kriptografi Bütünlügü Dogrulandi...",
        ">> Neural DWM Compositor & VirtIO Aygitlari Baslatiliyor...",
        ">> Hos Geldiniz: QuantroUser Calisma Alani Hazir..."
    };

    for (int step = 1; step <= 4; step++) {
        int fill = (bar_w * step) / 4;
        fb_fill_round_rect(bar_x, bar_y, fill, bar_h, 4, CLR_BLOOM_PETAL4);
        fb_blend_round_rect(bar_x + fill - 20, bar_y - 2, 24, bar_h + 4, 4, 0xFFFFFF, 180);

        fb_fill_rect(bar_x - 40, bar_y + 20, bar_w + 80, 24, 0x080E1C);
        fb_draw_text_centered(cx, bar_y + 24, stages[step - 1], 0x94A3B8, 1);

        /* Kısa bekleme gecikmesi */
        for (volatile int d = 0; d < 3500000; d++) {
            __asm__ volatile("nop");
        }
    }
}

/* ---- Bloom Duvar Kağıdı Motoru ------------------------------------ */

static void draw_photorealistic_bloom(void)
{
    /* Gece Mavisi Arka Plan */
    for (uint32_t y = 0; y < g_h; y++) {
        uint32_t t = (y * 255) / (g_h ? g_h : 1);
        uint32_t r = 2 + (t * 8) / 255;
        uint32_t g = 6 + (t * 18) / 255;
        uint32_t b = 18 + (t * 36) / 255;
        fb_fill_rect(0, (int)y, (int)g_w, 1, RGB(r, g, b));
    }

    int cx = (int)g_w / 2;
    int cy = (int)g_h / 2 - 20;

    /* Arka Plan Mor Parıltısı */
    for (int r = 360; r > 40; r -= 10) {
        uint8_t a = (uint8_t)((360 - r) * 45 / 360);
        fb_blend_round_rect(cx - r + 40, cy - r * 3 / 5, r * 2, r * 6 / 5, r / 2,
                            CLR_BLOOM_PURPLE, a);
    }

    /* Ana Mavi Bloom Işık Küresi */
    for (int r = 300; r > 30; r -= 8) {
        uint8_t a = (uint8_t)((300 - r) * 70 / 300);
        fb_blend_round_rect(cx - r, cy - r * 2 / 3, r * 2, r * 4 / 3, r / 2,
                            CLR_BLOOM_PETAL1, a);
    }

    /* Sol İpeksi Dalgalar */
    for (int i = 0; i < 45; i++) {
        int px = cx - 220 + i * 5;
        int py = cy + 60 - i * 6;
        int pw = 280 - i * 3;
        int ph = 190 - i * 2;
        fb_blend_round_rect(px, py, pw, ph, 50, CLR_BLOOM_PETAL3, 6);
    }

    /* Sağ İpeksi Dalgalar */
    for (int i = 0; i < 50; i++) {
        int px = cx - 30 + i * 5;
        int py = cy + 80 - i * 5;
        int pw = 300 - i * 4;
        int ph = 210 - i * 3;
        fb_blend_round_rect(px, py, pw, ph, 55, CLR_BLOOM_PETAL2, 7);
    }

    /* Merkez Parlak Çekirdek */
    for (int r = 160; r > 10; r -= 6) {
        uint8_t a = (uint8_t)((160 - r) * 95 / 160);
        fb_blend_round_rect(cx - r * 3 / 4, cy - r / 2, r * 3 / 2, r, r / 2,
                            CLR_BLOOM_GLOW, a);
    }
}

/* ---- Masaüstü Simgeleri ------------------------------------------- */

static void draw_desktop_icons(void)
{
    static const struct {
        const char *name;
        int app_id;
    } desk_apps[6] = {
        { "Bu Bilgisayar", APP_SETTINGS },
        { "Geri Donusum",  APP_EXPLORER },
        { "Dosyalar",      APP_EXPLORER },
        { "Microsoft Edge",APP_EDGE },
        { "PowerShell",    APP_TERMINAL },
        { "Hesap Makinesi",APP_CALC }
    };

    int start_x = 24;
    int start_y = 30;
    int step_y = 94;

    for (int i = 0; i < 6; i++) {
        int y = start_y + i * step_y;
        fb_blend_round_rect(start_x - 6, y - 6, 88, 80, 6, 0xFFFFFF, 12);

        int cx = start_x + 38;
        int cy = y + 26;

        switch (i) {
        case 0:
            fb_fill_round_rect(cx - 16, cy - 14, 32, 22, 3, 0x0078D4);
            fb_fill_round_rect(cx - 13, cy - 11, 26, 16, 2, 0x60CDFF);
            fb_fill_rect(cx - 4, cy + 8, 8, 5, 0xCCCCCC);
            fb_fill_rect(cx - 10, cy + 13, 20, 2, 0xCCCCCC);
            break;
        case 1:
            fb_fill_round_rect(cx - 14, cy - 14, 28, 28, 4, 0x555555);
            fb_draw_text(cx - 6, cy - 7, "♻", 0xFFFFFF, 1);
            break;
        case 2:
            draw_fluent_explorer(cx, cy, 32);
            break;
        case 3:
            draw_fluent_edge(cx, cy, 32);
            break;
        case 4:
            draw_fluent_terminal(cx, cy, 32);
            break;
        case 5:
            draw_fluent_calc(cx, cy, 32);
            break;
        }

        uint32_t tw = fb_text_width(desk_apps[i].name, 1);
        int tx = cx - (int)tw / 2;
        fb_draw_text(tx + 1, y + 54 + 1, desk_apps[i].name, 0x000000, 1);
        fb_draw_text(tx, y + 54, desk_apps[i].name, CLR_TEXT_WHITE, 1);
    }
}

/* ---- Modern Windows 11 Görev Çubuğu (Taskbar) --------------------- */

static void draw_win11_taskbar(void)
{
    int th = 50;
    int ty = (int)g_h - th;

    fb_draw_acrylic_panel(0, ty, (int)g_w, th, 0, 0x161616, 230);

    /* Sol Hava Durumu */
    fb_fill_round_rect(16, ty + 8, 140, 34, 6, 0x222222);
    fb_blend_round_rect(16, ty + 8, 140, 1, 1, 0xFFFFFF, 20);
    fb_draw_text(26, ty + 17, "☀ 21 C Bulutlu", CLR_TEXT_WHITE, 1);

    /* Ortalanmış Uygulamalar */
    int count = 7;
    int box_sz = 44;
    int start_x = (int)(g_w / 2) - ((count * box_sz) / 2);
    int cur_x = start_x;

    /* 1. Başlat */
    if (g_start_open) {
        fb_fill_round_rect(cur_x - 2, ty + 4, box_sz, box_sz, 6, 0x2A2A2A);
        fb_fill_round_rect(cur_x + 12, ty + th - 4, 16, 3, 1, CLR_ACCENT_BLUE);
    }
    draw_win_logo(cur_x + box_sz / 2, ty + th / 2, 24);
    cur_x += box_sz;

    /* 2. Edge */
    if (g_windows[APP_EDGE].open)
        fb_fill_round_rect(cur_x + 14, ty + th - 4, 16, 3, 1, CLR_ACCENT_BLUE);
    draw_fluent_edge(cur_x + box_sz / 2, ty + th / 2, 28);
    cur_x += box_sz;

    /* 3. Explorer */
    if (g_windows[APP_EXPLORER].open)
        fb_fill_round_rect(cur_x + 14, ty + th - 4, 16, 3, 1, CLR_ACCENT_BLUE);
    draw_fluent_explorer(cur_x + box_sz / 2, ty + th / 2, 28);
    cur_x += box_sz;

    /* 4. Store */
    draw_fluent_store(cur_x + box_sz / 2, ty + th / 2, 26);
    cur_x += box_sz;

    /* 5. Terminal */
    if (g_windows[APP_TERMINAL].open)
        fb_fill_round_rect(cur_x + 14, ty + th - 4, 16, 3, 1, CLR_ACCENT_BLUE);
    draw_fluent_terminal(cur_x + box_sz / 2, ty + th / 2, 26);
    cur_x += box_sz;

    /* 6. Calc */
    if (g_windows[APP_CALC].open)
        fb_fill_round_rect(cur_x + 14, ty + th - 4, 16, 3, 1, CLR_ACCENT_BLUE);
    draw_fluent_calc(cur_x + box_sz / 2, ty + th / 2, 26);
    cur_x += box_sz;

    /* 7. Settings */
    if (g_windows[APP_SETTINGS].open)
        fb_fill_round_rect(cur_x + 14, ty + th - 4, 16, 3, 1, CLR_ACCENT_BLUE);
    draw_fluent_settings(cur_x + box_sz / 2, ty + th / 2, 26);

    /* Sağ Sistem Tepsisi */
    int rx = (int)g_w - 24;
    char time_buf[16], date_buf[16];
    ksnprintf(time_buf, sizeof(time_buf), "%02d:%02d",
              ((g_boot_time + g_uptime) / 3600) % 24,
              ((g_boot_time + g_uptime) / 60) % 60);
    ksnprintf(date_buf, sizeof(date_buf), "25.10.2026");

    uint32_t tw = fb_text_width(date_buf, 1);
    rx -= (int)tw + 12;
    fb_draw_text(rx + 16, ty + 10, time_buf, CLR_TEXT_WHITE, 1);
    fb_draw_text(rx, ty + 28, date_buf, CLR_TEXT_MUTED, 1);

    rx -= 100;
    fb_fill_round_rect(rx, ty + 8, 92, 34, 17, 0x222222);
    fb_blend_round_rect(rx, ty + 8, 92, 1, 1, 0xFFFFFF, 20);
    fb_draw_text(rx + 12, ty + 17, "📶 🔊 🔋", CLR_TEXT_WHITE, 1);
}

/* ---- Başlat Menüsü (Start Menu) ----------------------------------- */

static void draw_win11_start_menu(int cx, int by)
{
    int mw = 580;
    int mh = 500;
    int mx = cx - mw / 2;
    int my = by - mh - 12;

    fb_blend_round_rect(mx - 8, my - 6, mw + 16, mh + 16, 14, 0x000000, 160);
    fb_draw_acrylic_panel(mx, my, mw, mh, 12, CLR_MICA_BG, 245);

    /* Arama Çubuğu */
    fb_fill_round_rect(mx + 32, my + 24, mw - 64, 38, 19, 0x2A2A2A);
    fb_blend_round_rect(mx + 32, my + 24, mw - 64, 1, 1, 0xFFFFFF, 20);
    fb_draw_text(mx + 54, my + 35, "🔍 Uygulamalari, dosyalari veya web'i arayin...", CLR_TEXT_MUTED, 1);

    /* Sabitlenen Uygulamalar Başlığı */
    fb_draw_text(mx + 36, my + 82, "Sabitlenenler", CLR_TEXT_WHITE, 1);
    fb_draw_text(mx + mw - 120, my + 82, "Tum Uygulamalar >", CLR_ACCENT_BLUE, 1);

    /* 18 Uygulama Izgarası (6 sütun x 3 satır) */
    static const struct {
        const char *name;
        int app_id;
    } apps[18] = {
        { "Edge", APP_EDGE }, { "Explorer", APP_EXPLORER }, { "Terminal", APP_TERMINAL },
        { "Hesap Mak.", APP_CALC }, { "Ayarlar", APP_SETTINGS }, { "Magaza", APP_SETTINGS },
        { "Word", APP_SETTINGS }, { "Excel", APP_SETTINGS }, { "PowerPoint", APP_SETTINGS },
        { "Mail", APP_SETTINGS }, { "Takvim", APP_SETTINGS }, { "Fotograflar", APP_SETTINGS },
        { "Not Defteri", APP_TERMINAL }, { "Paint", APP_SETTINGS }, { "Muzik", APP_SETTINGS },
        { "Gorev Yon.", APP_TASKMGR }, { "Hava Durumu", APP_SETTINGS }, { "Saat", APP_SETTINGS }
    };

    int grid_x = mx + 36;
    int grid_y = my + 110;
    int col_w = (mw - 72) / 6;
    int row_h = 70;

    for (int i = 0; i < 18; i++) {
        int r = i / 6;
        int c = i % 6;
        int ax = grid_x + c * col_w + col_w / 2;
        int ay = grid_y + r * row_h + 20;

        switch (i) {
        case 0: draw_fluent_edge(ax, ay, 28); break;
        case 1: draw_fluent_explorer(ax, ay, 28); break;
        case 2: draw_fluent_terminal(ax, ay, 28); break;
        case 3: draw_fluent_calc(ax, ay, 28); break;
        case 4: draw_fluent_settings(ax, ay, 28); break;
        case 5: draw_fluent_store(ax, ay, 28); break;
        case 15: draw_fluent_taskmgr(ax, ay, 28); break;
        default:
            fb_fill_round_rect(ax - 14, ay - 14, 28, 28, 6, 0x0078D4);
            fb_draw_text(ax - 4, ay - 7, "Q", 0xFFFFFF, 1);
            break;
        }

        uint32_t tw = fb_text_width(apps[i].name, 1);
        fb_draw_text(ax - (int)tw / 2, ay + 20, apps[i].name, CLR_TEXT_GRAY, 1);
    }

    /* Alt Kullanıcı Profili ve Güç Butonu */
    int foot_y = my + mh - 58;
    fb_fill_round_rect(mx, foot_y, mw, 58, 12, 0x181818);
    fb_blend_round_rect(mx, foot_y, mw, 1, 1, 0xFFFFFF, 25);

    fb_fill_round_rect(mx + 32, foot_y + 14, 30, 30, 15, CLR_ACCENT_BLUE);
    fb_draw_text(mx + 42, foot_y + 21, "Q", 0xFFFFFF, 1);
    fb_draw_text(mx + 72, foot_y + 16, "QuantroUser (Yonetici)", CLR_TEXT_WHITE, 1);
    fb_draw_text(mx + 72, foot_y + 34, "quantro@live.com", CLR_TEXT_MUTED, 1);

    int pwr_x = mx + mw - 56;
    fb_fill_round_rect(pwr_x, foot_y + 14, 32, 32, 6, 0x242424);
    fb_draw_text(pwr_x + 10, foot_y + 21, "⏻", 0xFF5555, 1);
}

/* ---- Pencere Çerçevesi Çizici ------------------------------------- */

static void draw_window_frame(int wx, int wy, int ww, int wh, const char *title, int app_id)
{
    fb_blend_round_rect(wx - 10, wy - 8, ww + 20, wh + 20, 16, 0x000000, 160);
    fb_draw_acrylic_panel(wx, wy, ww, wh, 10, CLR_MICA_BG, 240);

    /* Başlık Çubuğu */
    fb_fill_round_rect(wx, wy, ww, 42, 10, 0x242424);
    fb_fill_rect(wx, wy + 32, ww, 10, 0x242424);

    /* İkon */
    switch (app_id) {
    case APP_SETTINGS: draw_fluent_settings(wx + 22, wy + 21, 20); break;
    case APP_EXPLORER: draw_fluent_explorer(wx + 22, wy + 21, 20); break;
    case APP_CALC:     draw_fluent_calc(wx + 22, wy + 21, 20); break;
    case APP_TERMINAL: draw_fluent_terminal(wx + 22, wy + 21, 20); break;
    case APP_EDGE:     draw_fluent_edge(wx + 22, wy + 21, 20); break;
    case APP_TASKMGR:  draw_fluent_taskmgr(wx + 22, wy + 21, 20); break;
    }

    fb_draw_text(wx + 44, wy + 13, title, CLR_TEXT_WHITE, 1);

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
}

/* ---- Uygulama 1: Ayarlar Paneli ----------------------------------- */

static void render_app_settings(win_state_t *ws)
{
    draw_window_frame(ws->x, ws->y, ws->w, ws->h, "Ayarlar - Modern Sistem Yonetimi", APP_SETTINGS);

    int sb_w = 180;
    int sb_x = ws->x;
    int sb_y = ws->y + 42;
    int sb_h = ws->h - 42;

    fb_fill_rect(sb_x, sb_y, sb_w, sb_h, 0x222222);
    fb_blend_round_rect(sb_x + sb_w - 1, sb_y, 1, sb_h, 1, 0xFFFFFF, 20);

    static const char *tabs[4] = { "🖥 Sistem", "📶 Ag & Internet", "🎨 Kisisellestir", "🔒 Guvenlik & TPM" };
    int my = sb_y + 16;
    for (int t = 0; t < 4; t++) {
        if (g_settings_tab == t) {
            fb_fill_round_rect(sb_x + 10, my - 4, sb_w - 20, 34, 6, 0x333333);
            fb_fill_round_rect(sb_x + 10, my + 3, 3, 20, 1, CLR_ACCENT_BLUE);
            fb_draw_text(sb_x + 24, my + 4, tabs[t], CLR_TEXT_WHITE, 1);
        } else {
            fb_draw_text(sb_x + 24, my + 4, tabs[t], CLR_TEXT_GRAY, 1);
        }
        my += 40;
    }

    int cx = sb_x + sb_w + 24;
    int cy = sb_y + 16;
    int cw = ws->w - sb_w - 48;

    if (g_settings_tab == 0) {
        /* TAB 0: Sistem */
        draw_win_logo(cx + 24, cy + 24, 48);
        fb_draw_text(cx + 64, cy + 10, "QUANTRO-PC (ARM64 Virt)", CLR_TEXT_WHITE, 2);
        fb_draw_text(cx + 64, cy + 34, "Windows 11 Pro 24H2 | OS Derlemesi 26100.1742", CLR_TEXT_MUTED, 1);
        cy += 70;

        fb_fill_round_rect(cx, cy, cw, 78, 8, CLR_MICA_CARD);
        fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 25);
        fb_draw_text(cx + 16, cy + 8, "Cihaz Ozellikleri", CLR_ACCENT_CYAN, 1);
        fb_draw_text(cx + 16, cy + 28, "Islemci:       ARM Cortex-A57 @ 2.0 GHz (4 vCPU AArch64)", CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 16, cy + 46, "Bellek:        4096 MB RAM (PMM 4KB Granule + 2MB Kernel Heap)", CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 16, cy + 62, "Sistem Turu:   64-bit Isletim Sistemi, ARM tabanli islemci", CLR_TEXT_GRAY, 1);
        cy += 88;

        fb_fill_round_rect(cx, cy, cw, 78, 8, CLR_MICA_CARD);
        fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 25);
        fb_draw_text(cx + 16, cy + 8, "Cekirdek Durumu", 0x10B981, 1);
        fb_draw_text(cx + 16, cy + 28, "Preemptive Scheduler: Aktif (Hardware Context Switch)", CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 16, cy + 46, "Donanimsal Istisnalar: VBAR_EL1 Traps Calisiyor", CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 16, cy + 62, "VirtIO Input Tablet:  Slot 30 Etkin (Sifir Gecikme)", CLR_TEXT_GRAY, 1);
    } else if (g_settings_tab == 1) {
        /* TAB 1: Ağ & İnternet */
        fb_draw_text(cx, cy + 6, "Ag Baglantilari & NDIS Adaptorleri", CLR_TEXT_WHITE, 2);
        cy += 40;

        fb_fill_round_rect(cx, cy, cw, 100, 8, CLR_MICA_CARD);
        fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 25);
        fb_draw_text(cx + 16, cy + 10, "VirtIO Ethernet0 (10 Gbps High-Speed Link)", CLR_ACCENT_CYAN, 1);
        fb_draw_text(cx + 16, cy + 32, "IP Adresi:     192.168.1.105 (Subnet: 255.255.255.0)", CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 16, cy + 50, "Varsayilan Ag: 192.168.1.1 (Gateway Calisiyor)", CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 16, cy + 68, "DNS Sunucusu:  1.1.1.1, 8.8.8.8 (Cloudflare & Google)", CLR_TEXT_GRAY, 1);
        fb_draw_text(cx + 16, cy + 84, "MAC Adresi:    52:54:00:12:34:56 | Baglanti Durumu: LINK UP", 0x10B981, 1);
    } else if (g_settings_tab == 2) {
        /* TAB 2: Kişiselleştirme */
        fb_draw_text(cx, cy + 6, "Kisisellestirme & Tema Secimi", CLR_TEXT_WHITE, 2);
        cy += 40;

        fb_fill_round_rect(cx, cy, cw, 120, 8, CLR_MICA_CARD);
        fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 25);
        fb_draw_text(cx + 16, cy + 10, "Vurgu Rengi Secimi (Tiklanabilir)", CLR_ACCENT_CYAN, 1);

        uint32_t colors[4] = { 0x0078D4, 0x8B5CF6, 0x10B981, 0x60CDFF };
        const char *cnames[4] = { "Mavi", "Mor", "Yesil", "Cyan" };
        for (int c = 0; c < 4; c++) {
            int bx = cx + 20 + c * 90;
            fb_fill_round_rect(bx, cy + 36, 70, 50, 8, colors[c]);
            if (g_theme_accent == c) {
                fb_draw_round_border(bx - 3, cy + 33, 76, 56, 10, 2, 0xFFFFFF);
            }
            fb_draw_text(bx + 14, cy + 92, cnames[c], CLR_TEXT_GRAY, 1);
        }
    } else if (g_settings_tab == 3) {
        /* TAB 3: Güvenlik */
        fb_draw_text(cx, cy + 6, "Guvenlik & TPM 2.0 Standartlari", CLR_TEXT_WHITE, 2);
        cy += 40;

        fb_fill_round_rect(cx, cy, cw, 120, 8, CLR_MICA_CARD);
        fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 25);
        fb_draw_text(cx + 16, cy + 10, "Donanimsal TPM & Secure Boot Durumu", 0x10B981, 1);
        fb_draw_text(cx + 16, cy + 32, "[✔] TPM 2.0 PCR0-7 Kripto Dogrulama: AKTIF VE KILITLI", CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 16, cy + 50, "[✔] UEFI Secure Boot (PK, KEK, db):   ETKINLESTIRILDI", CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 16, cy + 68, "[✔] VBS & HVCI Cekirdek Korumasi:    TAM PARITE (W^X)", CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 16, cy + 86, "[✔] Quantro Defender Antivirus:      GERCEK ZAMANLI TARAMA AKTIF", 0x60CDFF, 1);
    }
}

/* ---- Uygulama 2: Dosya Gezgini (File Explorer) -------------------- */

static void render_app_explorer(win_state_t *ws)
{
    draw_window_frame(ws->x, ws->y, ws->w, ws->h, "Dosya Gezgini", APP_EXPLORER);

    int fx = ws->x;
    int fy = ws->y + 42;
    int fw = ws->w;

    /* Araç Çubuğu ve Adres Barı */
    fb_fill_rect(fx, fy, fw, 40, 0x222222);
    fb_blend_round_rect(fx, fy + 39, fw, 1, 1, 0xFFFFFF, 20);

    /* Geri / İleri Butonları */
    fb_fill_round_rect(fx + 12, fy + 8, 28, 24, 4, 0x333333);
    fb_draw_text(fx + 20, fy + 12, "<-", CLR_TEXT_WHITE, 1);

    fb_fill_round_rect(fx + 46, fy + 8, 28, 24, 4, 0x333333);
    fb_draw_text(fx + 54, fy + 12, "->", CLR_TEXT_MUTED, 1);

    /* Adres Çubuğu */
    static const char *paths[4] = {
        "C:\\",
        "C:\\Windows",
        "C:\\Windows\\System32",
        "C:\\Users\\Quantro"
    };
    fb_fill_round_rect(fx + 84, fy + 6, fw - 240, 28, 4, 0x1A1A1A);
    fb_blend_round_rect(fx + 84, fy + 6, fw - 240, 1, 1, 0xFFFFFF, 20);
    fb_draw_text(fx + 94, fy + 12, paths[g_explorer_path], CLR_TEXT_WHITE, 1);

    /* Arama */
    fb_fill_round_rect(fx + fw - 146, fy + 6, 134, 28, 4, 0x1A1A1A);
    fb_draw_text(fx + fw - 138, fy + 12, "🔍 Ara...", CLR_TEXT_MUTED, 1);

    /* Sol Ağaç Menü */
    int sb_w = 170;
    int sb_y = fy + 40;
    int sb_h = ws->h - 82;
    fb_fill_rect(fx, sb_y, sb_w, sb_h, 0x1E1E1E);
    fb_blend_round_rect(fx + sb_w - 1, sb_y, 1, sb_h, 1, 0xFFFFFF, 20);

    static const char *nav_tree[4] = { "📁 Bu Bilgisayar", "📁 C:\\ Sürücüsü", "📁 Belgeler", "📁 ESP Bolumu" };
    for (int i = 0; i < 4; i++) {
        if (g_explorer_path == i) {
            fb_fill_round_rect(fx + 6, sb_y + 12 + i * 36, sb_w - 12, 30, 4, 0x333333);
        }
        fb_draw_text(fx + 16, sb_y + 19 + i * 36, nav_tree[i], CLR_TEXT_WHITE, 1);
    }

    /* Sağ Klasör / Dosya Listesi */
    int lx = fx + sb_w + 16;
    int ly = sb_y + 16;

    static const char *files_c[5] = { "📁 Windows", "📁 Program Files", "📁 Users", "📄 kernel.elf (2.4 MB)", "⚙ config.sys" };
    static const char *files_win[4] = { "📁 System32", "📁 Drivers", "📄 explorer.exe", "📄 notepad.exe" };
    static const char *files_sys[5] = { "📄 ntdll.dll", "📄 kernel32.dll", "📄 dwm.exe", "📄 smss.exe", "📄 csrss.exe" };
    static const char *files_usr[4] = { "📁 Desktop", "📁 Documents", "📁 Downloads", "📄 profile.dat" };

    const char **flist = files_c;
    int fcount = 5;
    if (g_explorer_path == 1) { flist = files_win; fcount = 4; }
    else if (g_explorer_path == 2) { flist = files_sys; fcount = 5; }
    else if (g_explorer_path == 3) { flist = files_usr; fcount = 4; }

    for (int i = 0; i < fcount; i++) {
        int ry = ly + i * 34;
        if (g_explorer_selected == i) {
            fb_fill_round_rect(lx - 6, ry - 4, fw - sb_w - 36, 30, 4, 0x0078D4);
        }
        fb_draw_text(lx, ry + 2, flist[i], CLR_TEXT_WHITE, 1);
    }
}

/* ---- Uygulama 3: Hesap Makinesi (Calculator) ---------------------- */

static void render_app_calc(win_state_t *ws)
{
    draw_window_frame(ws->x, ws->y, ws->w, ws->h, "Hesap Makinesi", APP_CALC);

    int cx = ws->x + 20;
    int cy = ws->y + 54;
    int cw = ws->w - 40;

    /* Dijital LCD Ekran */
    fb_fill_round_rect(cx, cy, cw, 60, 6, 0x181818);
    fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 30);

    uint32_t dw = fb_text_width(g_calc_display, 2);
    fb_draw_text(cx + cw - (int)dw - 16, cy + 18, g_calc_display, 0x60CDFF, 2);

    /* Tuş Takımı */
    static const char *keys[20] = {
        "C", "(", ")", "/",
        "7", "8", "9", "*",
        "4", "5", "6", "-",
        "1", "2", "3", "+",
        "±", "0", ".", "="
    };

    int ky = cy + 74;
    int kw = (cw - 18) / 4;
    int kh = (ws->h - 180) / 5;

    for (int i = 0; i < 20; i++) {
        int r = i / 4;
        int c = i % 4;
        int kx = cx + c * (kw + 6);
        int kr = ky + r * (kh + 6);

        uint32_t bg = 0x2A2A2A;
        uint32_t fg = CLR_TEXT_WHITE;
        if (c == 3 || i == 19) { bg = 0x0078D4; }
        else if (i == 0) { bg = 0x552222; }

        fb_fill_round_rect(kx, kr, kw, kh, 6, bg);
        fb_blend_round_rect(kx, kr, kw, 1, 1, 0xFFFFFF, 25);

        uint32_t kw_txt = fb_text_width(keys[i], 1);
        fb_draw_text(kx + kw / 2 - (int)kw_txt / 2, kr + kh / 2 - 8, keys[i], fg, 1);
    }
}

/* ---- Uygulama 4: Windows Terminal / PowerShell -------------------- */

static void render_app_terminal(win_state_t *ws)
{
    draw_window_frame(ws->x, ws->y, ws->w, ws->h, "Windows PowerShell 7.4 (ARM64)", APP_TERMINAL);

    int tx = ws->x + 12;
    int ty = ws->y + 48;
    int tw = ws->w - 24;
    int th = ws->h - 100;

    fb_fill_round_rect(tx, ty, tw, th, 4, 0x0C0C0C);
    fb_blend_round_rect(tx, ty, tw, 1, 1, 0xFFFFFF, 20);

    /* Terminal Satırları */
    int ly = ty + 12;
    for (int i = 0; i < g_term_line_count && i < 12; i++) {
        uint32_t color = 0xCCCCCC;
        if (g_term_lines[i][0] == 'P' && g_term_lines[i][1] == 'S')
            color = 0x60CDFF;
        fb_draw_text(tx + 12, ly, g_term_lines[i], color, 1);
        ly += 20;
    }

    /* Tıklanabilir Komut Çipleri */
    int by = ty + th + 10;
    static const char *chips[5] = { "[sysinfo]", "[tasklist]", "[netstat]", "[tpm]", "[cls]" };
    int bx = tx;
    for (int i = 0; i < 5; i++) {
        fb_fill_round_rect(bx, by, 72, 28, 4, 0x222222);
        fb_blend_round_rect(bx, by, 72, 1, 1, 0xFFFFFF, 30);
        fb_draw_text(bx + 8, by + 6, chips[i], 0x60CDFF, 1);
        bx += 82;
    }
}

/* ---- Uygulama 5: Microsoft Edge ----------------------------------- */

static void render_app_edge(win_state_t *ws)
{
    draw_window_frame(ws->x, ws->y, ws->w, ws->h, "Microsoft Edge - Guvenli Web", APP_EDGE);

    int ex = ws->x;
    int ey = ws->y + 42;
    int ew = ws->w;

    /* Sekme Çubuğu */
    fb_fill_rect(ex, ey, ew, 32, 0x1E1E1E);
    static const char *edgetabs[3] = { "Quantro Hub", "GitHub", "Wikipedia" };
    for (int t = 0; t < 3; t++) {
        int tx = ex + 10 + t * 130;
        uint32_t bg = (g_edge_tab == t) ? 0x2B2B2B : 0x1E1E1E;
        fb_fill_round_rect(tx, ey + 4, 120, 28, 4, bg);
        fb_draw_text(tx + 12, ey + 10, edgetabs[t], CLR_TEXT_WHITE, 1);
    }

    /* URL Bar */
    ey += 32;
    fb_fill_rect(ex, ey, ew, 38, 0x252525);
    fb_fill_round_rect(ex + 16, ey + 6, ew - 32, 26, 13, 0x181818);
    fb_draw_text(ex + 32, ey + 11, "🔒 https://quantro.network/visionary (TLS 1.3)", 0x10B981, 1);

    /* Web İçerik Sayfası */
    ey += 38;
    int eh = ws->h - 112;
    fb_fill_rect(ex, ey, ew, eh, 0x121212);

    int cy = ey + 20;
    fb_draw_text(ex + 24, cy, "Quantro OS 2.0: Yeni Nesil Isletim Sistemi Devrimi", 0xFFFFFF, 2);
    cy += 34;

    fb_fill_round_rect(ex + 24, cy, ew - 48, 80, 8, CLR_MICA_CARD);
    fb_draw_text(ex + 40, cy + 12, "Donanim Paritesi ve AArch64 Cekirdek Mimarisi", CLR_ACCENT_CYAN, 1);
    fb_draw_text(ex + 40, cy + 34, "Quantro OS, Windows 11 Fluent arayuzunu bare-metal seviyesinde 64-bit", CLR_TEXT_WHITE, 1);
    fb_draw_text(ex + 40, cy + 52, "ARM islemcilerde native olarak calistirarak benzersiz bir deneyim sunar.", CLR_TEXT_GRAY, 1);
}

/* ---- Uygulama 6: Görev Yöneticisi (Task Manager) ------------------ */

static void render_app_taskmgr(win_state_t *ws)
{
    draw_window_frame(ws->x, ws->y, ws->w, ws->h, "Gorev Yoneticisi (Task Manager)", APP_TASKMGR);

    int cy = ws->y + 46;
    int cx = ws->x + 16;
    int cw = ws->w - 32;

    /* Sekmeler: Islemler, Performans, Baslangic */
    fb_fill_round_rect(cx, cy, 90, 26, 4, 0x333333);
    fb_fill_round_rect(cx, cy + 24, 90, 2, 1, CLR_ACCENT_BLUE);
    fb_draw_text(cx + 14, cy + 6, "Islemler", CLR_TEXT_WHITE, 1);

    fb_draw_text(cx + 104, cy + 6, "Performans", CLR_TEXT_GRAY, 1);
    fb_draw_text(cx + 200, cy + 6, "Baslangic", CLR_TEXT_GRAY, 1);

    cy += 34;

    /* Kolon Basliklari */
    fb_fill_round_rect(cx, cy, cw, 24, 4, 0x222222);
    fb_draw_text(cx + 10, cy + 5, "Ad (Islem)", CLR_TEXT_MUTED, 1);
    fb_draw_text(cx + 260, cy + 5, "Durum", CLR_TEXT_MUTED, 1);
    fb_draw_text(cx + 360, cy + 5, "CPU %", CLR_TEXT_MUTED, 1);
    fb_draw_text(cx + 450, cy + 5, "Bellek", CLR_TEXT_MUTED, 1);
    fb_draw_text(cx + 540, cy + 5, "Disk", CLR_TEXT_MUTED, 1);
    fb_draw_text(cx + 630, cy + 5, "GPU %", CLR_TEXT_MUTED, 1);

    cy += 28;

    static const struct {
        const char *name;
        const char *status;
        const char *cpu;
        const char *mem;
        const char *disk;
        const char *gpu;
        uint32_t heat_color;
    } proc_list[6] = {
        { "System Kernel (SMP/Limine)", "Calisiyor", "0.8%", "18.4 MB", "0.1 MB/s", "0.0%", 0x282828 },
        { "Mesa 3D Vulkan Compositor",  "Calisiyor", "4.2%", "142.0 MB", "0.4 MB/s", "12.5%", 0x2E2A1E },
        { "Quantro Shell & Taskbar",   "Calisiyor", "1.6%", "98.5 MB",  "0.0 MB/s", "3.1%", 0x282828 },
        { "Wine Proton 64-bit Core",    "Calisiyor", "8.5%", "256.0 MB", "1.8 MB/s", "18.2%", 0x3A281E },
        { "PipeWire Audio Server",      "Calisiyor", "0.4%", "32.1 MB",  "0.0 MB/s", "0.0%", 0x282828 },
        { "Storage Sense & Btrfs Auto", "Bosta",     "0.0%", "24.8 MB",  "0.0 MB/s", "0.0%", 0x282828 }
    };

    for (int p = 0; p < 6; p++) {
        fb_fill_round_rect(cx, cy, cw, 28, 4, proc_list[p].heat_color);
        fb_blend_round_rect(cx, cy, cw, 1, 1, 0xFFFFFF, 10);
        fb_draw_text(cx + 10, cy + 7, proc_list[p].name, CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 260, cy + 7, proc_list[p].status, 0x10B981, 1);
        fb_draw_text(cx + 360, cy + 7, proc_list[p].cpu, CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 450, cy + 7, proc_list[p].mem, CLR_TEXT_WHITE, 1);
        fb_draw_text(cx + 540, cy + 7, proc_list[p].disk, CLR_TEXT_GRAY, 1);
        fb_draw_text(cx + 630, cy + 7, proc_list[p].gpu, 0x60CDFF, 1);
        cy += 32;
    }

    /* Alt Durum Çubuğu */
    int foot_y = ws->y + ws->h - 38;
    fb_fill_rect(ws->x, foot_y, ws->w, 38, 0x202020);
    fb_blend_round_rect(ws->x, foot_y, ws->w, 1, 1, 0xFFFFFF, 15);
    fb_draw_text(cx + 10, foot_y + 11, "Islem Sayisi: 48 | Toplam CPU: %15.5 | Toplam RAM: 636 MB / 4096 MB", CLR_TEXT_MUTED, 1);

    int btn_x = ws->x + ws->w - 140;
    fb_fill_round_rect(btn_x, foot_y + 6, 124, 26, 4, CLR_ACCENT_BLUE);
    fb_draw_text(btn_x + 14, foot_y + 10, "Gorevi Sonlandir", CLR_TEXT_WHITE, 1);
}

/* ---- Hızlı Ayarlar Paneli (Win + A) ------------------------------- */

static void draw_win11_quick_settings(void)
{
    int pw = 360;
    int ph = 340;
    int px = (int)g_w - pw - 16;
    int py = (int)g_h - 50 - ph - 8;

    fb_blend_round_rect(px - 8, py - 6, pw + 16, ph + 16, 14, 0x000000, 160);
    fb_draw_acrylic_panel(px, py, pw, ph, 12, CLR_MICA_BG, 245);

    /* 6 Hızlı Düğme */
    static const struct {
        const char *icon;
        const char *name;
        int active;
    } q_btns[6] = {
        { "Wi-Fi", "Wi-Fi Bagli", 1 },
        { "BT", "Bluetooth", 1 },
        { "Mod", "Ucak Modu", 0 },
        { "Pil", "Pil Tasarrufu", 0 },
        { "Isik", "Gece Isigi", 1 },
        { "Eris", "Erisim", 0 }
    };

    int bx = px + 18;
    int by = py + 20;
    int bw = (pw - 48) / 2;
    int bh = 44;

    for (int i = 0; i < 6; i++) {
        int r = i / 2;
        int c = i % 2;
        int item_x = bx + c * (bw + 12);
        int item_y = by + r * (bh + 10);
        uint32_t bg_col = q_btns[i].active ? CLR_ACCENT_BLUE : 0x2A2A2A;

        fb_fill_round_rect(item_x, item_y, bw, bh, 6, bg_col);
        fb_draw_text(item_x + 10, item_y + 14, q_btns[i].icon, CLR_TEXT_WHITE, 1);
        fb_draw_text(item_x + 48, item_y + 14, q_btns[i].name, CLR_TEXT_WHITE, 1);
    }

    /* Parlaklık */
    int sy = py + 185;
    fb_draw_text(px + 20, sy, "Parlaklik", CLR_TEXT_MUTED, 1);
    fb_fill_round_rect(px + 20, sy + 18, pw - 40, 6, 3, 0x333333);
    fb_fill_round_rect(px + 20, sy + 18, (pw - 40) * 85 / 100, 6, 3, CLR_ACCENT_BLUE);
    fb_fill_round_rect(px + 20 + (pw - 40) * 85 / 100 - 8, sy + 13, 16, 16, 8, 0xFFFFFF);

    /* Ses */
    sy += 48;
    fb_draw_text(px + 20, sy, "Ses Seviyesi (PipeWire)", CLR_TEXT_MUTED, 1);
    fb_fill_round_rect(px + 20, sy + 18, pw - 40, 6, 3, 0x333333);
    fb_fill_round_rect(px + 20, sy + 18, (pw - 40) * 75 / 100, 6, 3, CLR_ACCENT_BLUE);
    fb_fill_round_rect(px + 20 + (pw - 40) * 75 / 100 - 8, sy + 13, 16, 16, 8, 0xFFFFFF);

    /* Alt Kısım */
    int foot_y = py + ph - 42;
    fb_fill_round_rect(px, foot_y, pw, 42, 12, 0x1A1A1A);
    fb_draw_text(px + 18, foot_y + 14, "Pil: %100 Dolu | 8 Saat Kaldi", CLR_TEXT_WHITE, 1);
    fb_draw_text(px + pw - 85, foot_y + 14, "Ayarlar", CLR_ACCENT_CYAN, 1);
}

/* ---- Bildirim Merkezi ve Takvim (Win + N) ------------------------- */

static void draw_win11_notification_center(void)
{
    int nw = 360;
    int nh = 460;
    int nx = (int)g_w - nw - 16;
    int ny = (int)g_h - 50 - nh - 8;

    fb_blend_round_rect(nx - 8, ny - 6, nw + 16, nh + 16, 14, 0x000000, 160);
    fb_draw_acrylic_panel(nx, ny, nw, nh, 12, CLR_MICA_BG, 245);

    /* Bildirim Başlığı */
    fb_draw_text(nx + 20, ny + 18, "Bildirimler", CLR_TEXT_WHITE, 2);
    fb_draw_text(nx + nw - 95, ny + 22, "Tumunu Sil", CLR_ACCENT_BLUE, 1);

    /* Bildirim Kartı 1 */
    int cy = ny + 54;
    fb_fill_round_rect(nx + 16, cy, nw - 32, 64, 8, CLR_MICA_CARD);
    fb_blend_round_rect(nx + 16, cy, nw - 32, 1, 1, 0xFFFFFF, 20);
    fb_draw_text(nx + 28, cy + 10, "Quantro Guvenlik", CLR_ACCENT_CYAN, 1);
    fb_draw_text(nx + 28, cy + 28, "TPM 2.0 & LUKS2 Butunlugu Dogrulandi", CLR_TEXT_WHITE, 1);
    fb_draw_text(nx + 28, cy + 44, "Sistem guvenli ve izole modda calisiyor.", CLR_TEXT_MUTED, 1);

    /* Bildirim Kartı 2 */
    cy += 74;
    fb_fill_round_rect(nx + 16, cy, nw - 32, 64, 8, CLR_MICA_CARD);
    fb_blend_round_rect(nx + 16, cy, nw - 32, 1, 1, 0xFFFFFF, 20);
    fb_draw_text(nx + 28, cy + 10, "Windows Update", CLR_ACCENT_BLUE, 1);
    fb_draw_text(nx + 28, cy + 28, "Quantro OS 24H2 Yamalari Guncel", CLR_TEXT_WHITE, 1);
    fb_draw_text(nx + 28, cy + 44, "Btrfs anlik kurtarma noktasi hazir.", CLR_TEXT_MUTED, 1);

    /* Takvim Kartı (Ekim 2026) */
    cy += 78;
    fb_fill_round_rect(nx + 16, cy, nw - 32, 170, 8, 0x1A1A1A);
    fb_draw_text(nx + 28, cy + 12, "Ekim 2026", CLR_TEXT_WHITE, 1);
    fb_draw_text(nx + nw - 110, cy + 12, "Pazar, 25", CLR_ACCENT_CYAN, 1);

    /* Gün İsimleri */
    static const char *days[7] = { "Pt", "Sa", "Ca", "Pe", "Cu", "Ct", "Pz" };
    for (int d = 0; d < 7; d++) {
        fb_draw_text(nx + 32 + d * 42, cy + 36, days[d], CLR_TEXT_MUTED, 1);
    }

    /* Gün Numaraları */
    int cal_day = 1;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 7; col++) {
            if (cal_day <= 31) {
                int dx = nx + 32 + col * 42;
                int dy = cy + 58 + row * 24;
                char d_str[4];
                ksnprintf(d_str, sizeof(d_str), "%2d", cal_day);
                if (cal_day == 25) {
                    fb_fill_round_rect(dx - 4, dy - 3, 24, 20, 10, CLR_ACCENT_BLUE);
                    fb_draw_text(dx, dy, d_str, 0xFFFFFF, 1);
                } else {
                    fb_draw_text(dx, dy, d_str, CLR_TEXT_GRAY, 1);
                }
                cal_day++;
            }
        }
    }
}

/* ---- Fare İmleci Çizici ------------------------------------------- */

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

    /* İmleç Gölgesi */
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (cursor_data[y][x] != ' ') {
                fb_blend_rect(mx + x + 2, my + y + 2, 1, 1, 0x000000, 160);
            }
        }
    }

    /* İmleç Gövdesi */
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

static void restore_cursor_bg(void)
{
    if (!s_has_saved_bg || s_saved_cx < 0 || s_saved_cy < 0) return;
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 20; x++) {
            fb_put_pixel(s_saved_cx + x, s_saved_cy + y, s_cursor_bg[y][x]);
        }
    }
    s_has_saved_bg = 0;
}

static void save_cursor_bg(int mx, int my)
{
    s_saved_cx = mx;
    s_saved_cy = my;
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 20; x++) {
            s_cursor_bg[y][x] = fb_get_pixel(mx + x, my + y);
        }
    }
    s_has_saved_bg = 1;
}

/* ---- Başlatma ve Genel Çizim -------------------------------------- */

void gui_init(void)
{
    g_w = fb_width();
    g_h = fb_height();
    if (g_w == 0 || g_h == 0) {
        g_w = 1280;
        g_h = 800;
    }

    g_cursor_x = (int)g_w / 2;
    g_cursor_y = (int)g_h / 2;
    g_start_open = 0;
    g_active_app = APP_SETTINGS;

    /* Pencerelerin Başlangıç Boyutları */
    for (int i = 1; i <= 6; i++) {
        g_windows[i].open = (i == APP_SETTINGS) ? 1 : 0;
        g_windows[i].minimized = 0;
        g_windows[i].maximized = 0;
        g_windows[i].w = 760;
        g_windows[i].h = 440;
        g_windows[i].x = ((int)g_w / 2) - (g_windows[i].w / 2) + (i - 1) * 20;
        g_windows[i].y = 70 + (i - 1) * 15;
        g_windows[i].orig_x = g_windows[i].x;
        g_windows[i].orig_y = g_windows[i].y;
        g_windows[i].orig_w = g_windows[i].w;
        g_windows[i].orig_h = g_windows[i].h;
    }

    /* Hesap Makinesi daha derli toplu olsun */
    g_windows[APP_CALC].w = 340;
    g_windows[APP_CALC].h = 420;
    g_windows[APP_CALC].x = (int)g_w / 2 - 170;
    g_windows[APP_CALC].y = 100;

    /* Görev Yöneticisi */
    g_windows[APP_TASKMGR].w = 780;
    g_windows[APP_TASKMGR].h = 460;
    g_windows[APP_TASKMGR].x = ((int)g_w / 2) - (g_windows[APP_TASKMGR].w / 2);
    g_windows[APP_TASKMGR].y = 80;
}

void gui_redraw_all(void)
{
    if (!fb_ready()) return;

    s_has_saved_bg = 0;

    /* 1. Arka Plan Duvar Kağıdı */
    draw_photorealistic_bloom();

    /* 2. Masaüstü Simgeleri */
    draw_desktop_icons();

    /* 3. Aktif Uygulama Penceresi */
    if (g_active_app != APP_NONE && g_windows[g_active_app].open && !g_windows[g_active_app].minimized) {
        win_state_t *ws = &g_windows[g_active_app];
        switch (g_active_app) {
        case APP_SETTINGS: render_app_settings(ws); break;
        case APP_EXPLORER: render_app_explorer(ws); break;
        case APP_CALC:     render_app_calc(ws); break;
        case APP_TERMINAL: render_app_terminal(ws); break;
        case APP_EDGE:     render_app_edge(ws); break;
        case APP_TASKMGR:  render_app_taskmgr(ws); break;
        }
    }

    /* 4. Başlat Menüsü */
    if (g_start_open) {
        draw_win11_start_menu((int)g_w / 2, (int)g_h - 50);
    }

    /* 5. Görev Çubuğu */
    draw_win11_taskbar();

    /* 6. Hızlı Ayarlar (Win+A) */
    if (g_quick_settings_open) {
        draw_win11_quick_settings();
    }

    /* 7. Bildirim Merkezi & Takvim (Win+N) */
    if (g_notifications_open) {
        draw_win11_notification_center();
    }

    /* 6. Dinamik Fare İmleci */
    save_cursor_bg(g_cursor_x, g_cursor_y);
    draw_win11_aero_cursor(g_cursor_x, g_cursor_y);
}

void gui_move_cursor(int x, int y)
{
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int)g_w) x = (int)g_w - 1;
    if (y >= (int)g_h) y = (int)g_h - 1;

    if (x == g_cursor_x && y == g_cursor_y) return;

    restore_cursor_bg();
    g_cursor_x = x;
    g_cursor_y = y;
    save_cursor_bg(g_cursor_x, g_cursor_y);
    draw_win11_aero_cursor(g_cursor_x, g_cursor_y);
}

void gui_open_app(int app_id)
{
    if (app_id < 1 || app_id > 5) return;
    g_active_app = app_id;
    g_windows[app_id].open = 1;
    g_windows[app_id].minimized = 0;
    g_start_open = 0;
    gui_redraw_all();
}

void gui_toggle_app(int app_id)
{
    if (app_id < 1 || app_id > 5) return;
    if (g_active_app == app_id && g_windows[app_id].open && !g_windows[app_id].minimized) {
        g_windows[app_id].minimized = 1;
    } else {
        gui_open_app(app_id);
    }
    gui_redraw_all();
}

void gui_close_active_app(void)
{
    if (g_active_app != APP_NONE) {
        g_windows[g_active_app].open = 0;
        g_active_app = APP_NONE;
        gui_redraw_all();
    }
}

void gui_toggle_start_menu(void)
{
    g_start_open = !g_start_open;
    gui_redraw_all();
}

void gui_toggle_settings_window(void)
{
    gui_toggle_app(APP_SETTINGS);
}

void gui_toggle_quick_settings(void)
{
    g_quick_settings_open = !g_quick_settings_open;
    if (g_quick_settings_open) {
        g_notifications_open = 0;
        g_start_open = 0;
    }
    gui_redraw_all();
}

void gui_toggle_notifications(void)
{
    g_notifications_open = !g_notifications_open;
    if (g_notifications_open) {
        g_quick_settings_open = 0;
        g_start_open = 0;
    }
    gui_redraw_all();
}

/* ---- Etkileşimli Tıklama İşleyicisi (Interactivity Router) -------- */

void gui_handle_mouse(int x, int y, int btn)
{
    gui_move_cursor(x, y);

    if (btn != 0) {
        int th = 50;
        int ty = (int)g_h - th;

        /* 1. GÖREV ÇUBUĞU TIKLAMALARI */
        if (y >= ty && y < (int)g_h) {
            int count = 7;
            int box_sz = 44;
            int start_x = (int)(g_w / 2) - ((count * box_sz) / 2);

            for (int i = 0; i < count; i++) {
                int bx = start_x + i * box_sz;
                if (x >= bx && x < bx + box_sz) {
                    if (i == 0) { /* Başlat */
                        gui_toggle_start_menu();
                        return;
                    } else if (i == 1) { /* Edge */
                        gui_toggle_app(APP_EDGE);
                        return;
                    } else if (i == 2) { /* Explorer */
                        gui_toggle_app(APP_EXPLORER);
                        return;
                    } else if (i == 3) { /* Store / Ayarlar */
                        gui_toggle_app(APP_SETTINGS);
                        return;
                    } else if (i == 4) { /* Terminal */
                        gui_toggle_app(APP_TERMINAL);
                        return;
                    } else if (i == 5) { /* Calculator */
                        gui_toggle_app(APP_CALC);
                        return;
                    } else if (i == 6) { /* Settings */
                        gui_toggle_app(APP_SETTINGS);
                        return;
                    }
                }
            }

            /* Sağ Sistem Tepsisi Tıklamaları */
            if (x >= (int)g_w - 220 && x < (int)g_w - 110) {
                gui_toggle_quick_settings();
                return;
            }
            if (x >= (int)g_w - 110) {
                gui_toggle_notifications();
                return;
            }
            return;
        }

        /* 2. BAŞLAT MENÜSÜ İÇİ TIKLAMALARI */
        if (g_start_open) {
            int mw = 580;
            int mh = 500;
            int mx = (int)g_w / 2 - mw / 2;
            int my = ty - mh - 12;

            if (x >= mx && x < mx + mw && y >= my && y < my + mh) {
                /* Uygulama ızgarası */
                int grid_x = mx + 36;
                int grid_y = my + 110;
                int col_w = (mw - 72) / 6;
                int row_h = 70;

                for (int i = 0; i < 18; i++) {
                    int r = i / 6;
                    int c = i % 6;
                    int ax = grid_x + c * col_w;
                    int ay = grid_y + r * row_h;
                    if (x >= ax && x < ax + col_w && y >= ay && y < ay + row_h) {
                        static const int target_app[18] = {
                            APP_EDGE, APP_EXPLORER, APP_TERMINAL, APP_CALC, APP_SETTINGS, APP_SETTINGS,
                            APP_SETTINGS, APP_SETTINGS, APP_SETTINGS, APP_SETTINGS, APP_SETTINGS, APP_SETTINGS,
                            APP_TERMINAL, APP_SETTINGS, APP_SETTINGS, APP_TASKMGR, APP_SETTINGS, APP_SETTINGS
                        };
                        gui_open_app(target_app[i]);
                        return;
                    }
                }
                return;
            } else {
                g_start_open = 0;
                gui_redraw_all();
                return;
            }
        }

        /* 3. AKTİF PENCERE TIKLAMALARI */
        if (g_active_app != APP_NONE && g_windows[g_active_app].open && !g_windows[g_active_app].minimized) {
            win_state_t *ws = &g_windows[g_active_app];

            /* Başlık Çubuğu Butonları */
            if (y >= ws->y && y < ws->y + 42) {
                int close_x = ws->x + ws->w - 46;
                if (x >= close_x && x < close_x + 38) {
                    gui_close_active_app();
                    return;
                }
                int max_x = close_x - 42;
                if (x >= max_x && x < max_x + 38) {
                    if (ws->maximized) {
                        ws->x = ws->orig_x;
                        ws->y = ws->orig_y;
                        ws->w = ws->orig_w;
                        ws->h = ws->orig_h;
                        ws->maximized = 0;
                    } else {
                        ws->orig_x = ws->x; ws->orig_y = ws->y;
                        ws->orig_w = ws->w; ws->orig_h = ws->h;
                        ws->x = 10; ws->y = 10;
                        ws->w = (int)g_w - 20; ws->h = ty - 20;
                        ws->maximized = 1;
                    }
                    gui_redraw_all();
                    return;
                }
                int min_x = max_x - 42;
                if (x >= min_x && x < min_x + 38) {
                    ws->minimized = 1;
                    gui_redraw_all();
                    return;
                }

                /* Başlık Çubuğundan Sürükleme / Taşıma */
                if (x >= ws->x && x < min_x) {
                    ws->x = x - ws->w / 2;
                    ws->y = y - 20;
                    if (ws->x < 10) ws->x = 10;
                    if (ws->y < 10) ws->y = 10;
                    if (ws->x + ws->w > (int)g_w - 10) ws->x = (int)g_w - ws->w - 10;
                    if (ws->y + ws->h > ty - 10) ws->y = ty - ws->h - 10;
                    gui_redraw_all();
                    return;
                }
            }

            /* Pencere İçi İnteraktif Tıklamalar */
            if (x >= ws->x && x < ws->x + ws->w && y >= ws->y + 42 && y < ws->y + ws->h) {
                if (g_active_app == APP_SETTINGS) {
                    /* Ayarlar Yan Menü Tabları */
                    int sb_x = ws->x;
                    int sb_w = 180;
                    int sb_y = ws->y + 42;
                    if (x >= sb_x && x < sb_x + sb_w) {
                        int tab = (y - (sb_y + 12)) / 40;
                        if (tab >= 0 && tab < 4) {
                            g_settings_tab = tab;
                            gui_redraw_all();
                            return;
                        }
                    }
                    /* Vurgu Rengi Tıklaması */
                    if (g_settings_tab == 2 && y >= ws->y + 120 && y <= ws->y + 190) {
                        int cx = sb_x + sb_w + 24;
                        for (int c = 0; c < 4; c++) {
                            int bx = cx + 20 + c * 90;
                            if (x >= bx && x < bx + 70) {
                                g_theme_accent = c;
                                gui_redraw_all();
                                return;
                            }
                        }
                    }
                } else if (g_active_app == APP_EXPLORER) {
                    /* Sol Menü Tıklaması */
                    int sb_w = 170;
                    if (x >= ws->x && x < ws->x + sb_w) {
                        int path_idx = (y - (ws->y + 94)) / 36;
                        if (path_idx >= 0 && path_idx < 4) {
                            g_explorer_path = path_idx;
                            g_explorer_selected = -1;
                            gui_redraw_all();
                            return;
                        }
                    }
                    /* Dosya Listesi Tıklaması */
                    int lx = ws->x + sb_w + 16;
                    if (x >= lx && x < ws->x + ws->w - 20) {
                        int sel = (y - (ws->y + 98)) / 34;
                        if (sel >= 0 && sel < 5) {
                            g_explorer_selected = sel;
                            if (g_explorer_path == 0 && sel == 0) g_explorer_path = 1; /* Windows */
                            else if (g_explorer_path == 1 && sel == 0) g_explorer_path = 2; /* System32 */
                            gui_redraw_all();
                            return;
                        }
                    }
                } else if (g_active_app == APP_CALC) {
                    /* Hesap Makinesi Tuş Tıklamaları */
                    int cx = ws->x + 20;
                    int cy = ws->y + 54;
                    int cw = ws->w - 40;
                    int ky = cy + 74;
                    int kw = (cw - 18) / 4;
                    int kh = (ws->h - 180) / 5;

                    static const char key_chars[20] = {
                        'C', '(', ')', '/',
                        '7', '8', '9', '*',
                        '4', '5', '6', '-',
                        '1', '2', '3', '+',
                        '#', '0', '.', '='
                    };

                    for (int i = 0; i < 20; i++) {
                        int r = i / 4;
                        int c = i % 4;
                        int kx = cx + c * (kw + 6);
                        int kr = ky + r * (kh + 6);
                        if (x >= kx && x < kx + kw && y >= kr && y < kr + kh) {
                            char kc = key_chars[i];
                            if (kc == 'C') {
                                g_calc_display[0] = '0';
                                g_calc_display[1] = '\0';
                                g_calc_accum = 0;
                                g_calc_op = 0;
                                g_calc_fresh = 1;
                            } else if (kc >= '0' && kc <= '9') {
                                if (g_calc_fresh || (g_calc_display[0] == '0' && g_calc_display[1] == '\0')) {
                                    g_calc_display[0] = kc;
                                    g_calc_display[1] = '\0';
                                    g_calc_fresh = 0;
                                } else {
                                    int len = (int)strlen(g_calc_display);
                                    if (len < 12) {
                                        g_calc_display[len] = kc;
                                        g_calc_display[len + 1] = '\0';
                                    }
                                }
                            } else if (kc == '+' || kc == '-' || kc == '*' || kc == '/') {
                                g_calc_accum = atoi(g_calc_display);
                                g_calc_op = kc;
                                g_calc_fresh = 1;
                            } else if (kc == '=') {
                                int curr = atoi(g_calc_display);
                                int res = curr;
                                if (g_calc_op == '+') res = g_calc_accum + curr;
                                else if (g_calc_op == '-') res = g_calc_accum - curr;
                                else if (g_calc_op == '*') res = g_calc_accum * curr;
                                else if (g_calc_op == '/' && curr != 0) res = g_calc_accum / curr;
                                ksnprintf(g_calc_display, sizeof(g_calc_display), "%d", res);
                                g_calc_op = 0;
                                g_calc_fresh = 1;
                            }
                            gui_redraw_all();
                            return;
                        }
                    }
                } else if (g_active_app == APP_TERMINAL) {
                    /* Terminal Komut Çipleri */
                    int tx = ws->x + 12;
                    int ty = ws->y + 48;
                    int th = ws->h - 100;
                    int by = ty + th + 10;
                    int bx = tx;
                    static const char *chip_cmds[5] = { "sysinfo", "tasklist", "netstat", "tpm", "cls" };
                    for (int i = 0; i < 5; i++) {
                        if (x >= bx && x < bx + 72 && y >= by && y < by + 28) {
                            if (i == 4) { /* cls */
                                g_term_line_count = 2;
                            } else {
                                if (g_term_line_count < 10) {
                                    ksnprintf(g_term_lines[g_term_line_count++], 80, "PS C:\\Users\\Quantro> %s", chip_cmds[i]);
                                    if (i == 0)
                                        ksnprintf(g_term_lines[g_term_line_count++], 80, "OS: Windows 11 AArch64 | RAM: 4096MB | CPU: 4x A57");
                                    else if (i == 1)
                                        ksnprintf(g_term_lines[g_term_line_count++], 80, "PID 1: smss.exe | PID 2: csrss.exe | PID 4: dwm.exe");
                                    else if (i == 2)
                                        ksnprintf(g_term_lines[g_term_line_count++], 80, "TCP 0.0.0.0:80 [LISTENING] | NDIS 10Gbps [UP]");
                                    else if (i == 3)
                                        ksnprintf(g_term_lines[g_term_line_count++], 80, "TPM 2.0 PCR0-7: [0x5A4D...VERIFIED_LOCKED]");
                                }
                            }
                            gui_redraw_all();
                            return;
                        }
                        bx += 82;
                    }
                } else if (g_active_app == APP_EDGE) {
                    /* Edge Sekme Tıklaması */
                    if (y >= ws->y + 42 && y <= ws->y + 74) {
                        int tab = (x - (ws->x + 10)) / 130;
                        if (tab >= 0 && tab < 3) {
                            g_edge_tab = tab;
                            gui_redraw_all();
                            return;
                        }
                    }
                }
                return;
            }
        }

        /* 4. MASAÜSTÜ SİMGELERİNE TIKLAMA */
        int start_x = 24;
        int start_y = 30;
        int step_y = 94;
        static const int desk_target[6] = {
            APP_SETTINGS, APP_EXPLORER, APP_EXPLORER, APP_EDGE, APP_TERMINAL, APP_CALC
        };
        for (int i = 0; i < 6; i++) {
            int iy = start_y + i * step_y;
            if (x >= start_x - 6 && x < start_x + 82 && y >= iy - 6 && y < iy + 74) {
                gui_open_app(desk_target[i]);
                return;
            }
        }
    }
}

int gui_get_cursor_x(void) { return g_cursor_x; }
int gui_get_cursor_y(void) { return g_cursor_y; }

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
    if (message)
        fb_draw_text(80, 340, message, 0xBAE6FD, 1);
}
