/*
 * Quantro OS - Masaustu Etkilesim ve Grafik Arayuzu
 */
#ifndef QUANTRO_GUI_H
#define QUANTRO_GUI_H

#include "types.h"

void gui_init(void);
void gui_redraw_all(void);
void gui_tick(int64_t boot_time, uint64_t uptime_seconds);
void gui_panic(const char *message);

/* Etkilesimli Fare ve Pencere Olaylari */
void gui_handle_mouse(int x, int y, int btn);
void gui_move_cursor(int x, int y);
void gui_toggle_start_menu(void);
void gui_toggle_settings_window(void);
int  gui_get_cursor_x(void);
int  gui_get_cursor_y(void);

#endif /* QUANTRO_GUI_H */
