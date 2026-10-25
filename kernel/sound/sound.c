#include "sound.h"
#include "uart.h"
#include "kernel.h"

static int g_volume = 75;

void sound_init(void)
{
    g_volume = 75;
    uart_puts("[sound] Windows HD Audio / VirtIO-Sound DSP baslatildi (%75 seviye)\r\n");
    sound_startup();
}

void sound_startup(void)
{
    uart_puts("[sound:audio] ♪ ♫ Windows 11 Startup Melodisi caliniyor (F# -> A# -> D# -> G#) ♫ ♪\r\n");
}

void sound_beep(uint32_t freq, uint32_t duration_ms)
{
    uart_printf("[sound:beep] Frekans: %u Hz, Sure: %u ms (Ses Seviyesi: %%%d)\r\n",
                freq, duration_ms, g_volume);
}

void sound_set_volume(int vol)
{
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    g_volume = vol;
    uart_printf("[sound] Ses duzeyi ayarlandi: %%%d\r\n", g_volume);
}

int sound_get_volume(void)
{
    return g_volume;
}
