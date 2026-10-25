#include "clipboard.h"
#include "uart.h"
#include "kernel.h"

static char clip_buf[256];
static int  clip_len = 0;

void clipboard_init(void)
{
    clip_buf[0] = '\0';
    clip_len = 0;
    uart_puts("[wm:clipboard] Windows 11 Smart Clipboard (Panoya Kopyalama) hazir\r\n");
}

int clip_copy(const char *text)
{
    if (!text)
        return -1;
    strncpy(clip_buf, text, sizeof(clip_buf) - 1);
    clip_buf[sizeof(clip_buf) - 1] = '\0';
    clip_len = strlen(clip_buf);
    uart_printf("[clip] %d bayt panoya kopyalandi.\r\n", clip_len);
    return 0;
}

const char *clip_paste(void)
{
    return clip_buf;
}

void clip_status(void)
{
    uart_printf("Pano Icerigi (%d bayt): \"%s\"\r\n", clip_len, clip_buf);
}
