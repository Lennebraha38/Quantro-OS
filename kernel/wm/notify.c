#include "notify.h"
#include "uart.h"
#include "kernel.h"

static notify_item_t notifs[MAX_NOTIFICATIONS];
static int notif_count = 0;

void notify_init(void)
{
    notif_count = 0;
    notify_push("Guvenlik Merkezi", "TPM 2.0 ve Secure Boot etkinlestirildi.");
    notify_push("Ag Baglantisi", "Ethernet0 uzerinden yerel aga baglandi.");
    uart_puts("[wm:notify] Windows 11 Action Center / Toast Bildirim Motoru hazir\r\n");
}

void notify_push(const char *title, const char *msg)
{
    if (notif_count >= MAX_NOTIFICATIONS) {
        for (int i = 0; i < MAX_NOTIFICATIONS - 1; i++)
            notifs[i] = notifs[i + 1];
        notif_count = MAX_NOTIFICATIONS - 1;
    }
    int idx = notif_count++;
    strncpy(notifs[idx].title, title, 31);
    notifs[idx].title[31] = '\0';
    strncpy(notifs[idx].msg, msg, 63);
    notifs[idx].msg[63] = '\0';
    notifs[idx].timestamp = (uint32_t)(timer_ticks() / (timer_frequency() ? timer_frequency() : 1));

    uart_printf("[toast] %s: %s\r\n", title, msg);
}

void notify_list(void)
{
    uart_puts("\r\n--- Windows 11 Bildirim Merkezi (Toast / Action Center) ---\r\n");
    if (notif_count == 0) {
        uart_puts(" Yeni bildirim yok.\r\n");
        return;
    }
    for (int i = notif_count - 1; i >= 0; i--) {
        uart_printf(" [%d saniye once] %s\r\n   -> %s\r\n",
                    notifs[i].timestamp, notifs[i].title, notifs[i].msg);
    }
    uart_puts("-----------------------------------------------------------\r\n");
}

void notify_clear(void)
{
    notif_count = 0;
    uart_puts("[notify] Tum bildirimler temizlendi.\r\n");
}
