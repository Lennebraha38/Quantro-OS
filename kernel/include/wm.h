#ifndef QUANTRO_WM_H
#define QUANTRO_WM_H

#include "types.h"

#define MAX_WINDOWS 8

typedef struct {
    int  id;
    int  x, y, w, h;
    int  visible;
    int  focused;
    char title[32];
} win_t;

void wm_init(void);
int  wm_create_window(const char *title);
void wm_list(void);
int  wm_focus(int id);
int  wm_close(int id);
void wm_handle_key(char c);
void wm_handle_mouse(int x, int y, int btn);
int  wm_window_count(void);
const win_t *wm_get_window(int index);

#endif /* QUANTRO_WM_H */
