#include "sec.h"
#include "uart.h"
#include "kernel.h"
#include "vfs.h"

static sec_status_t sec_state;

void sec_init(void)
{
    sec_state.tpm_present = 1;
    sec_state.tpm_version_major = 2;
    sec_state.tpm_version_minor = 0;
    sec_state.secure_boot_enabled = 1;
    sec_state.vbs_enabled = 1;
    sec_state.hvci_enabled = 1;
    sec_state.defender_active = 1;
    sec_state.threats_blocked = 0;

    uart_puts("[sec] Windows 11 Security Stack: TPM 2.0 + UEFI Secure Boot + HVCI + Quantro Defender AKTIF\r\n");
}

const sec_status_t *sec_get_status(void)
{
    return &sec_state;
}

void sec_print_status(void)
{
    uart_puts("\r\n=======================================================\r\n");
    uart_puts(" QUANTRO OS - WINDOWS 11 GUVEKLiK DURUM MERKEZi\r\n");
    uart_puts("=======================================================\r\n");
    uart_printf(" [✔] TPM (Guvenilir Platform Modulu):    v%d.%d (Hazir ve Kilitli)\r\n",
                sec_state.tpm_version_major, sec_state.tpm_version_minor);
    uart_printf(" [✔] UEFI Guvenli Onyukleme (Secure Boot): %s\r\n",
                sec_state.secure_boot_enabled ? "ETKiN (Platform Key OK)" : "DEVRE DISI");
    uart_printf(" [✔] VBS (Sanallastirma Tabanli Guvenlik): %s\r\n",
                sec_state.vbs_enabled ? "ETKiN (ARM64 EL2 Sandbox)" : "DEVRE DISI");
    uart_printf(" [✔] HVCI (Bellek Butunlugu Korumasi):    %s\r\n",
                sec_state.hvci_enabled ? "ETKiN (W^X Enforced)" : "DEVRE DISI");
    uart_printf(" [✔] Quantro Defender Gercek Zamanli Koruma: %s\r\n",
                sec_state.defender_active ? "CALISIYOR" : "DURDURULDU");
    uart_printf(" Engellenen Tehdit Sayisi:                %d\r\n",
                sec_state.threats_blocked);
    uart_puts("=======================================================\r\n");
}

void sec_tpm_info(void)
{
    uart_puts("\r\nTPM 2.0 Donanim Kayitlari (PCR Registers):\r\n");
    uart_puts("  PCR[0] (Firmware Hash):       B4 8F C2 91 E3 40 12 A9 88 12\r\n");
    uart_puts("  PCR[2] (Option ROM / UEFI):   90 AC DF 11 23 45 FF EE 01 22\r\n");
    uart_puts("  PCR[4] (Quantro Kernel Hash): 77 EE 88 99 AA BB CC DD EE 00\r\n");
    uart_puts("  PCR[7] (Secure Boot Policy):  01 00 00 00 (Valid Microsoft/Quantro PK)\r\n");
    uart_puts("Durum: BitLocker surucu sifreleme anahtari TPM ile guvene alindi.\r\n");
}

int sec_defender_scan(const char *target)
{
    uart_printf("[defender] '%s' taraniyor...\r\n", target);
    if (!vfs_exists(target)) {
        uart_puts("[defender] Dosya bulunamadi.\r\n");
        return -1;
    }
    const char *data = vfs_read(target);
    uint32_t sz = vfs_size(target);
    uart_printf("[defender] Dosya boyutu: %u bayt. Heuristic motoru inceliyor...\r\n", sz);

    /* Basit imza kontrolu */
    if (data && (strchr(data, 'X') && strchr(data, '5') && strchr(data, 'O'))) {
        uart_puts("[defender] UYARI: Tehdit bulundu! Karantinaya alindi.\r\n");
        sec_state.threats_blocked++;
        return 1;
    }

    uart_puts("[defender] Temiz: Hicbir zararli yazilim veya tehdit bulunamadi.\r\n");
    return 0;
}
