#include "wm.h"
#include "uart.h"
#include "kernel.h"

static win_t windows[MAX_WINDOWS];
static int win_count = 0;

void wm_init(void)
{
    win_count = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].id = i;
        windows[i].visible = 0;
        windows[i].focused = 0;
        windows[i].title[0] = '\0';
    }
    uart_puts("[wm] Windows 11 DWM (Desktop Window Manager) Compositor hazir\r\n");
}

int wm_create_window(const char *title)
{
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].visible) {
            windows[i].id = i;
            windows[i].visible = 1;
            windows[i].focused = 1;
            windows[i].x = 60 + i * 30;
            windows[i].y = 50 + i * 25;
            windows[i].w = 640;
            windows[i].h = 420;
            strncpy(windows[i].title, title, 31);
            windows[i].title[31] = '\0';

            /* Diger pencerelerin odagini kaldir */
            for (int j = 0; j < MAX_WINDOWS; j++) {
                if (j != i)
                    windows[j].focused = 0;
            }

            if (i >= win_count)
                win_count = i + 1;

            uart_printf("[wm] Pencere olusturuldu [%d]: %s (640x420)\r\n", i, windows[i].title);
            return i;
        }
    }
    uart_puts("[wm] Pencere tablosu dolu!\r\n");
    return -1;
}

void wm_list(void)
{
    uart_puts("\r\n Aktif Masaustu Pencereleri (DWM Tasklist):\r\n");
    uart_puts(" ID  Konum       Boyut     Odak  Baslik\r\n");
    uart_puts(" -----------------------------------------------------\r\n");
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].visible) {
            uart_printf(" [%d] (%3d,%3d)   %3dx%-3d   %s   %s\r\n",
                        windows[i].id,
                        windows[i].x, windows[i].y,
                        windows[i].w, windows[i].h,
                        windows[i].focused ? " [*] " : " [ ] ",
                        windows[i].title);
        }
    }
}

int wm_focus(int id)
{
    if (id < 0 || id >= MAX_WINDOWS || !windows[id].visible)
        return -1;

    for (int i = 0; i < MAX_WINDOWS; i++)
        windows[i].focused = (i == id);

    uart_printf("[wm] Pencere [%d] odaklandi: %s\r\n", id, windows[id].title);
    return 0;
}

int wm_close(int id)
{
    if (id < 0 || id >= MAX_WINDOWS || !windows[id].visible)
        return -1;

    windows[id].visible = 0;
    windows[id].focused = 0;
    uart_printf("[wm] Pencere [%d] kapatildi: %s\r\n", id, windows[id].title);
    return 0;
}

void wm_handle_key(char c)
{
    (void)c;
}

void wm_handle_mouse(int x, int y, int btn)
{
    (void)x; (void)y; (void)btn;
}

int wm_window_count(void)
{
    int c = 0;
    for (int i = 0; i < MAX_WINDOWS; i++)
        if (windows[i].visible) c++;
    return c;
}

const win_t *wm_get_window(int index)
{
    if (index >= 0 && index < MAX_WINDOWS && windows[index].visible)
        return &windows[index];
    return 0;
}
