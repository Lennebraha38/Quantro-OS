#ifndef WM_H
#define WM_H
void wm_init(void);
void wm_create_window(const char *title);
void wm_handle_key(char c);
void wm_handle_mouse(int x,int y,int btn);
#endif
