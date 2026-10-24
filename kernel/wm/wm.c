#include "wm.h"
#include "uart.h"
/* Pencere yoneticisi iskeleti: compositor + olay kuyrugu */
typedef struct { int x,y,w,h; char title[32]; } window_t;
static window_t windows[8];
static int win_count=0;
void wm_init(void){ win_count=0; uart_puts("[wm] compositor init (stub, max 8 windows)\r\n"); }
void wm_create_window(const char *title){
    if(win_count>=8){ uart_puts("[wm] window table full\r\n"); return; }
    windows[win_count].x=10+win_count*20;
    windows[win_count].y=10+win_count*20;
    windows[win_count].w=320; windows[win_count].h=200;
    int i=0; while(title[i] && i<31){ windows[win_count].title[i]=title[i]; i++; } windows[win_count].title[i]='\0';
    uart_printf("[wm] create window %d: %s (stub)\r\n", win_count, windows[win_count].title);
    win_count++;
}
void wm_handle_key(char c){ (void)c; uart_puts("[wm] key event (stub)\r\n"); }
void wm_handle_mouse(int x,int y,int btn){ (void)x;(void)y;(void)btn; uart_puts("[wm] mouse event (stub)\r\n"); }
