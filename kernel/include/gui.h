/*
 * Quantro OS - Masaüstü Etkileşim ve Gelişmiş Grafik Arayüzü Motoru
 */
#ifndef QUANTRO_GUI_H
#define QUANTRO_GUI_H

#include "types.h"

enum {
    APP_NONE = 0,
    APP_SETTINGS = 1,
    APP_EXPLORER = 2,
    APP_CALC = 3,
    APP_TERMINAL = 4,
    APP_EDGE = 5
};

void gui_init(void);
void gui_show_visionary_boot(void);
void gui_redraw_all(void);
void gui_tick(int64_t boot_time, uint64_t uptime_seconds);
void gui_panic(const char *message);

/* Uygulama ve Pencere Yönetimi */
void gui_open_app(int app_id);
void gui_toggle_app(int app_id);
void gui_close_active_app(void);
void gui_toggle_start_menu(void);
void gui_toggle_settings_window(void);

/* Etkileşimli Fare ve İmleç Olayları */
void gui_handle_mouse(int x, int y, int btn);
void gui_move_cursor(int x, int y);
int  gui_get_cursor_x(void);
int  gui_get_cursor_y(void);

#endif /* QUANTRO_GUI_H */
