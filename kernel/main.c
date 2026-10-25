/*
 * Quantro OS - Ana Cekirdek (AArch64, Limine Protokolu rev 2)
 * Windows 11 Mimari ve Fonksiyonel Esitlik Surumu
 */
#include "types.h"
#include "kernel.h"
#include "uart.h"
#include "fb.h"
#include "gui.h"
#include "vfs.h"
#include "net.h"
#include "sound.h"
#include "proc.h"
#include "user.h"
#include "sec.h"
#include "wm.h"
#include "clipboard.h"
#include "notify.h"
#include "calculator.h"
#include "settings.h"
#include "filemgr.h"
#include "browser.h"
#include "terminal.h"
#include "limine.h"

extern uint64_t psci_version(void);
extern void psci_reset(void);

/* ---- Limine istekleri (base protocol revision 2) ------------------ */

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_boot_time_request boot_time_request = {
    .id = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

/* ---- Genel durum -------------------------------------------------- */

static volatile uintptr_t hhdm_offset = 0;
static int64_t boot_unix_time = 0;
static uint64_t memmap_usable_bytes = 0;

/* ---- Bellek haritasi ---------------------------------------------- */

static const char *memmap_type_name(uint64_t type)
{
    switch (type) {
    case LIMINE_MEMMAP_USABLE:                 return "usable";
    case LIMINE_MEMMAP_RESERVED:               return "reserved";
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:       return "acpi_reclaim";
    case LIMINE_MEMMAP_ACPI_NVS:               return "acpi_nvs";
    case LIMINE_MEMMAP_BAD_MEMORY:             return "bad_memory";
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: return "reclaimable";
    case LIMINE_MEMMAP_KERNEL_AND_MODULES:     return "kernel_and_modules";
    case LIMINE_MEMMAP_FRAMEBUFFER:            return "framebuffer";
    default:                                   return "unknown";
    }
}

static void shell_mem(void)
{
    if (!memmap_request.response) {
        uart_puts("Bellek haritasi mevcut degil.\r\n");
        return;
    }

    struct limine_memmap_response *resp = memmap_request.response;
    uart_printf("Bellek giris sayisi: %u\r\n", (unsigned)resp->entry_count);

    uint64_t total = 0;
    for (uint64_t i = 0; i < resp->entry_count; i++) {
        struct limine_memmap_entry *e = resp->entries[i];
        uint64_t mb = e->length / (1024 * 1024);
        const char *name = memmap_type_name(e->type);

        uart_printf("[%llu] base=0x%llx len=%llu MiB type=%s\r\n",
                    (unsigned long long)i,
                    (unsigned long long)e->base,
                    (unsigned long long)mb,
                    name);

        if (e->type == LIMINE_MEMMAP_USABLE)
            total += e->length;
    }
    memmap_usable_bytes = total;
    uart_printf("Toplam Kullanilabilir RAM: %u MiB\r\n",
                (unsigned)(memmap_usable_bytes / (1024 * 1024)));
}

/* ---- Sistem Bilgisi (Windows Systeminfo) --------------------------- */

static void shell_sysinfo(void)
{
    uart_puts("\r\n=======================================================\r\n");
    uart_puts(" QUANTRO OS - WINDOWS 11 SISTEM BILGISI (SYSTEMINFO)\r\n");
    uart_puts("=======================================================\r\n");
    uart_puts(" Ana Bilgisayar Adi:          QUANTRO-PC\r\n");
    uart_puts(" Isletim Sistemi Adi:         Microsoft Windows 11 Pro (Quantro Kernel)\r\n");
    uart_printf(" Isletim Sistemi Surumu:      10.0.26100 Yapisi 26100 (v%s)\r\n", KERNEL_VERSION);
    uart_puts(" OS Ureticisi:                Quantro OpenSource Core\r\n");
    uart_puts(" Sistem Mimarisi:             ARM64-based PC (AArch64)\r\n");
    uart_puts(" Onyukleme Aygi:              \\Device\\HarddiskVolume1 (Limine UEFI rev 2)\r\n");
    uart_puts(" BIOS / Firmware:             EDK II UEFI ARM64 v2025.11\r\n");
    uart_puts(" Guvenli Onyukleme Durumu:    ACIK (Secure Boot Validated)\r\n");
    uart_puts(" Donanim Tabanli Guvenlik:    TPM 2.0 (Etkin ve Kilitli)\r\n");
    uart_puts(" Sanallastirma Guvenligi:     VBS & HVCI Calisiyor\r\n");
    uart_printf(" Toplam Fiziksel Bellek:      %u MiB\r\n",
                (unsigned)(memmap_usable_bytes / (1024 * 1024)));
    uart_puts(" Ag Bagdastiricisi:           VirtIO-Net 10Gbps Ethernet0 (192.168.1.105)\r\n");
    uart_puts("=======================================================\r\n");
}

/* ---- UART yardimci ------------------------------------------------ */

static void uart_print_info(void)
{
    uart_printf("\r\n=======================================================\r\n");
    uart_printf(" %s (Windows 11 Edition) v%s\r\n", KERNEL_NAME, KERNEL_VERSION);
    uart_printf(" Mimari: %s | Onyukleme: Limine Protokolu rev 2\r\n", KERNEL_ARCH);
    if (framebuffer_request.response &&
        framebuffer_request.response->framebuffer_count > 0) {
        struct limine_framebuffer *fb =
            framebuffer_request.response->framebuffers[0];
        uart_printf(" Ekran: %ux%u (%u bpp) TrueColor\r\n",
                    (unsigned)fb->width, (unsigned)fb->height, (unsigned)fb->bpp);
    }
    uart_printf(" Kullanilabilir RAM: %u MiB\r\n",
                (unsigned)(memmap_usable_bytes / (1024 * 1024)));
    uart_printf(" Oturum: %s\\%s\r\n", "QUANTRO-PC", user_current_name());
    uart_printf(" Komut listesi icin 'help' veya '?' yazin.\r\n");
    uart_printf("=======================================================\r\n\r\n");
    uart_printf("PS C:\\Users\\Quantro> ");
}

/* ---- Kabuk (Shell) Yardimi ---------------------------------------- */

static void shell_help(void)
{
    uart_puts(
        "\r\n--- QUANTRO OS KOMUT KILAVUZU (Windows 11 / PowerShell Parity) ---\r\n\r\n"
        " SISTEM & DONANIM:\r\n"
        "   help, ?                 - Bu yardim ekranini gosterir\r\n"
        "   ver                     - Isletim sistemi surumunu gosterir\r\n"
        "   sysinfo, systeminfo     - Detayli sistem bilesenleri raporu\r\n"
        "   mem                     - Limine bellek haritasini listeler\r\n"
        "   fb                      - Framebuffer ekran bilgilerini gosterir\r\n"
        "   uptime                  - Calisma suresini gosterir\r\n"
        "   cls, clear              - Ekrani temizler\r\n"
        "   reboot                  - Sistemi yeniden baslatir (ARM PSCI)\r\n"
        "   halt                    - Cekirdegi durdurur\r\n\r\n"
        " DOSYA SISTEMI (NTFS / VFS):\r\n"
        "   dir, ls [yol]           - Dosyalari ve dizinleri listeler\r\n"
        "   type, cat <dosya>       - Dosya icerigini okur\r\n"
        "   touch <dosya>           - Yeni bos dosya olusturur\r\n"
        "   write <dosya> <metin>   - Dosyaya metin yazar\r\n"
        "   append <dosya> <metin>  - Dosyanin sonuna metin ekler\r\n"
        "   del, rm <dosya>         - Dosyayi siler\r\n"
        "   ren, mv <eski> <yeni>   - Dosyayi yeniden adlandirir\r\n"
        "   copy, cp <kaynak> <hed> - Dosyayi kopyalar\r\n"
        "   mkdir, md <dizin>       - Yeni dizin olusturur\r\n"
        "   stat <dosya>            - Dosya boyutunu ve durumunu verir\r\n\r\n"
        " GOREV & SUREC YONETIMI (TASK MANAGER):\r\n"
        "   tasklist, ps            - Calisan tum surecleri listeler\r\n"
        "   taskkill <pid>          - Belirtilen sureci sonlandirir\r\n"
        "   start <ad>              - Yeni bir surec baslatir\r\n\r\n"
        " GUVENLIK & KULLANICI (SECURITY & SAM):\r\n"
        "   whoami                  - Gecerli kullanici ve SID bilgisini verir\r\n"
        "   users                   - Sistem kullanicilarini listeler\r\n"
        "   login <kull> <parola>   - Oturum degistirir\r\n"
        "   useradd <kull> <parola> - Yeni kullanici olusturur\r\n"
        "   secstat                 - TPM 2.0, Secure Boot ve VBS durumu\r\n"
        "   tpm                     - TPM PCR donanim kayitlarini gosterir\r\n"
        "   defender <dosya>        - Dosyayi zararli yazilim taramasindan gecirir\r\n\r\n"
        " AG & ILETISIM (NETWORKING):\r\n"
        "   ipconfig, ifconfig      - IP yapilandirmasi ve NDIS adaptor\r\n"
        "   ping <ip>               - ICMP echo istegi gonderir\r\n"
        "   netstat                 - Etkin TCP/UDP portlarini listeler\r\n"
        "   curl, fetch <url>       - Web sayfasina HTTP istegi atar\r\n\r\n"
        " UYGULAMALAR & PENCERE YONETICISI:\r\n"
        "   calc <sayi> <op> <sayi> - Modern hesap makinesi (+, -, *, /, %%)\r\n"
        "   settings                - Windows 11 Ayarlar Paneli\r\n"
        "   explorer                - Dosya Gezgini arayuzunu acar\r\n"
        "   edge <url>              - Microsoft Edge web tarayicisini calistirir\r\n"
        "   powershell              - Windows PowerShell basligini acar\r\n"
        "   wm                      - Aktif DWM pencerelerini listeler\r\n"
        "   toast <baslik> <mesaj>  - Windows Action Center bildirimi gonderir\r\n"
        "   notifications           - Gelen bildirimleri listeler\r\n"
        "   clip [metin]            - Panoya kopyalar veya panoyu gosterir\r\n"
        "   volume [0-100]          - Sistem ses seviyesini ayarlar\r\n"
        "   beep                    - Uyari sesi calar\r\n\r\n");
}

static void shell_fb(void)
{
    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count < 1) {
        uart_puts("Ekran bilgisi yok.\r\n");
        return;
    }
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    uart_printf("Cozunurluk:  %u x %u piksel\r\n", (unsigned)fb->width, (unsigned)fb->height);
    uart_printf("Satir Boyu:  %u bayt\r\n", (unsigned)fb->pitch);
    uart_printf("Renk Derin.: %u bpp (TrueColor)\r\n", (unsigned)fb->bpp);
    uart_printf("FB Adresi:   0x%p\r\n", (void *)(uintptr_t)fb->address);
}

static void shell_uptime(void)
{
    uint64_t ticks = timer_ticks();
    uint64_t freq = timer_frequency();
    uint64_t sec = freq ? (ticks / freq) : 0;
    uart_printf("Sistem Calisma Suresi: %02llu saat, %02llu dakika, %02llu saniye\r\n",
                (unsigned long long)(sec / 3600),
                (unsigned long long)((sec / 60) % 60),
                (unsigned long long)(sec % 60));
}

static void shell_reboot(void)
{
    uart_puts("PSCI SYSTEM_RESET cagriliyor, sistem yeniden baslatiliyor...\r\n");
    uint64_t ver = psci_version();
    if (ver != 0)
        psci_reset();
    uart_puts("Yeniden baslatma gerceklestirilemedi.\r\n");
}

/* ---- Komut Calistirici -------------------------------------------- */

static void shell_execute(char *line)
{
    while (*line == ' ')
        line++;

    if (*line == '\0')
        goto prompt;

    /* Parametreleri ayristir */
    char cmd[32];
    char arg1[64];
    char arg2[64];
    char arg3[64];
    cmd[0] = '\0';
    arg1[0] = '\0';
    arg2[0] = '\0';
    arg3[0] = '\0';

    int i = 0;
    while (*line && *line != ' ' && i < 31)
        cmd[i++] = *line++;
    cmd[i] = '\0';

    while (*line == ' ') line++;
    i = 0;
    while (*line && *line != ' ' && i < 63)
        arg1[i++] = *line++;
    arg1[i] = '\0';

    while (*line == ' ') line++;
    i = 0;
    while (*line && *line != ' ' && i < 63)
        arg2[i++] = *line++;
    arg2[i] = '\0';

    while (*line == ' ') line++;
    i = 0;
    while (*line && i < 63)
        arg3[i++] = *line++;
    arg3[i] = '\0';

    /* Komut karsilastirmalari */
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        shell_help();
    } else if (strcmp(cmd, "ver") == 0) {
        uart_printf("Microsoft Windows [Surum 10.0.26100.1742] (%s v%s %s)\r\n",
                    KERNEL_NAME, KERNEL_VERSION, KERNEL_ARCH);
    } else if (strcmp(cmd, "sysinfo") == 0 || strcmp(cmd, "systeminfo") == 0) {
        shell_sysinfo();
    } else if (strcmp(cmd, "mem") == 0) {
        shell_mem();
    } else if (strcmp(cmd, "fb") == 0) {
        shell_fb();
    } else if (strcmp(cmd, "uptime") == 0) {
        shell_uptime();
    } else if (strcmp(cmd, "cls") == 0 || strcmp(cmd, "clear") == 0) {
        uart_puts("\033[2J\033[H");
    } else if (strcmp(cmd, "reboot") == 0) {
        shell_reboot();
    } else if (strcmp(cmd, "halt") == 0 || strcmp(cmd, "exit") == 0) {
        uart_puts("Sistem durduruluyor...\r\n");
        halt_forever();
    }
    /* DOSYA SISTEMI */
    else if (strcmp(cmd, "dir") == 0 || strcmp(cmd, "ls") == 0) {
        if (arg1[0] != '\0')
            vfs_list_dir(arg1);
        else
            vfs_list();
    } else if (strcmp(cmd, "type") == 0 || strcmp(cmd, "cat") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: type <dosya>\r\n");
        } else {
            const char *data = vfs_read(arg1);
            if (data)
                uart_printf("%s\r\n", data);
            else
                uart_printf("Hata: '%s' dosyasi bulunamadi.\r\n", arg1);
        }
    } else if (strcmp(cmd, "touch") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: touch <dosya>\r\n");
        } else {
            if (vfs_create(arg1) == 0)
                uart_printf("Dosya olusturuldu: %s\r\n", arg1);
            else
                uart_puts("Hata: Dosya tablosu dolu veya olusturulamadi.\r\n");
        }
    } else if (strcmp(cmd, "write") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            uart_puts("Kullanim: write <dosya> <metin>\r\n");
        } else {
            vfs_write(arg1, arg2);
            uart_printf("'%s' dosyasina yazildi.\r\n", arg1);
        }
    } else if (strcmp(cmd, "append") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            uart_puts("Kullanim: append <dosya> <metin>\r\n");
        } else {
            vfs_append(arg1, arg2);
            uart_printf("'%s' dosyasina eklendi.\r\n", arg1);
        }
    } else if (strcmp(cmd, "del") == 0 || strcmp(cmd, "rm") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: del <dosya>\r\n");
        } else {
            if (vfs_delete(arg1) == 0)
                uart_printf("'%s' silindi.\r\n", arg1);
            else
                uart_printf("Hata: '%s' bulunamadi.\r\n", arg1);
        }
    } else if (strcmp(cmd, "ren") == 0 || strcmp(cmd, "mv") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            uart_puts("Kullanim: ren <eski> <yeni>\r\n");
        } else {
            if (vfs_rename(arg1, arg2) == 0)
                uart_printf("'%s' -> '%s' olarak adlandirildi.\r\n", arg1, arg2);
            else
                uart_puts("Hata: Yeniden adlandirma basarisiz.\r\n");
        }
    } else if (strcmp(cmd, "copy") == 0 || strcmp(cmd, "cp") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            uart_puts("Kullanim: copy <kaynak> <hedef>\r\n");
        } else {
            if (vfs_copy(arg1, arg2) == 0)
                uart_printf("Dosya kopyalandi: %s -> %s\r\n", arg1, arg2);
            else
                uart_puts("Hata: Kopyalama basarisiz.\r\n");
        }
    } else if (strcmp(cmd, "mkdir") == 0 || strcmp(cmd, "md") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: mkdir <klasor>\r\n");
        } else {
            vfs_mkdir(arg1);
            uart_printf("Klasor olusturuldu: %s\r\n", arg1);
        }
    } else if (strcmp(cmd, "stat") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: stat <dosya>\r\n");
        } else {
            if (vfs_exists(arg1))
                uart_printf("'%s': Boyut = %u bayt\r\n", arg1, vfs_size(arg1));
            else
                uart_printf("'%s' mevcut degil.\r\n", arg1);
        }
    }
    /* GOREV & SUREC YONETIMI */
    else if (strcmp(cmd, "tasklist") == 0 || strcmp(cmd, "ps") == 0) {
        proc_list();
    } else if (strcmp(cmd, "taskkill") == 0 || strcmp(cmd, "kill") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: taskkill <pid>\r\n");
        } else {
            int pid = atoi(arg1);
            if (proc_kill(pid) == 0)
                uart_printf("Basari: PID %d islemi sonlandirildi.\r\n", pid);
            else
                uart_printf("Hata: PID %d sonlandirilamadi (gecersiz veya korunuyor).\r\n", pid);
        }
    } else if (strcmp(cmd, "start") == 0 || strcmp(cmd, "spawn") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: start <program>\r\n");
        } else {
            int pid = proc_spawn(arg1);
            if (pid >= 0)
                uart_printf("Surec baslatildi: %s (PID: %d)\r\n", arg1, pid);
            else
                uart_puts("Hata: Surec tablosu dolu.\r\n");
        }
    }
    /* GUVENLIK & KULLANICI */
    else if (strcmp(cmd, "whoami") == 0) {
        user_whoami();
    } else if (strcmp(cmd, "users") == 0) {
        user_list();
    } else if (strcmp(cmd, "login") == 0 || strcmp(cmd, "su") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0')
            uart_puts("Kullanim: login <kullanici> <parola>\r\n");
        else
            user_login(arg1, arg2);
    } else if (strcmp(cmd, "useradd") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0')
            uart_puts("Kullanim: useradd <kullanici> <parola>\r\n");
        else
            user_add(arg1, arg2, USER_ROLE_USER);
    } else if (strcmp(cmd, "secstat") == 0 || strcmp(cmd, "security") == 0) {
        sec_print_status();
    } else if (strcmp(cmd, "tpm") == 0) {
        sec_tpm_info();
    } else if (strcmp(cmd, "defender") == 0 || strcmp(cmd, "scan") == 0) {
        if (arg1[0] == '\0')
            uart_puts("Kullanim: defender <dosya>\r\n");
        else
            sec_defender_scan(arg1);
    }
    /* AG */
    else if (strcmp(cmd, "ipconfig") == 0 || strcmp(cmd, "ifconfig") == 0) {
        net_ipconfig();
    } else if (strcmp(cmd, "ping") == 0) {
        if (arg1[0] == '\0')
            net_ping("8.8.8.8");
        else
            net_ping(arg1);
    } else if (strcmp(cmd, "netstat") == 0) {
        net_netstat();
    } else if (strcmp(cmd, "curl") == 0 || strcmp(cmd, "fetch") == 0) {
        if (arg1[0] == '\0')
            net_fetch("http://quantro-os.org");
        else
            net_fetch(arg1);
    }
    /* UYGULAMALAR & DWM */
    else if (strcmp(cmd, "calc") == 0) {
        if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
            calculator_run();
        } else {
            int n1 = atoi(arg1);
            char op = arg2[0];
            int n2 = atoi(arg3);
            int res = 0;
            if (calculator_calc(n1, op, n2, &res) == 0)
                uart_printf("%d %c %d = %d\r\n", n1, op, n2, res);
        }
    } else if (strcmp(cmd, "settings") == 0) {
        settings_run();
    } else if (strcmp(cmd, "explorer") == 0 || strcmp(cmd, "filemgr") == 0) {
        filemgr_run();
    } else if (strcmp(cmd, "edge") == 0 || strcmp(cmd, "browser") == 0) {
        if (arg1[0] != '\0')
            browser_navigate(arg1);
        else
            browser_run();
    } else if (strcmp(cmd, "powershell") == 0 || strcmp(cmd, "cmd") == 0) {
        terminal_run();
    } else if (strcmp(cmd, "wm") == 0) {
        wm_list();
    } else if (strcmp(cmd, "toast") == 0) {
        if (arg1[0] == '\0')
            notify_push("Sistem", "Yeni bildirim alindi.");
        else
            notify_push(arg1, arg2[0] ? arg2 : "Quantro Bildirim Mesaji");
    } else if (strcmp(cmd, "notifications") == 0) {
        notify_list();
    } else if (strcmp(cmd, "clip") == 0) {
        if (arg1[0] == '\0')
            clip_status();
        else
            clip_copy(arg1);
    } else if (strcmp(cmd, "volume") == 0) {
        if (arg1[0] == '\0')
            uart_printf("Gecerli Ses Seviyesi: %%%d\r\n", sound_get_volume());
        else
            sound_set_volume(atoi(arg1));
    } else if (strcmp(cmd, "beep") == 0) {
        sound_beep(1000, 200);
    } else if (strcmp(cmd, "echo") == 0) {
        uart_printf("%s %s %s\r\n", arg1, arg2, arg3);
    } else {
        uart_printf("'%s' komutu bulunamadi. Desteklenen komutlar icin 'help' yazin.\r\n", cmd);
    }

prompt:
    uart_printf("PS C:\\Users\\Quantro> ");
}

static void shell_loop(void)
{
    char line[128];
    size_t len = 0;

    uart_print_info();

    for (;;) {
        char c = uart_getc();

        if (c == '\r')
            c = '\n';

        if (c == '\n') {
            line[len] = '\0';
            shell_execute(line);
            len = 0;
            continue;
        }

        if (c == 127 || c == 8) {
            if (len > 0) {
                len--;
                uart_puts("\b \b");
            }
            continue;
        }

        if (len < sizeof(line) - 1 && c >= 32 && c < 127) {
            line[len++] = c;
            uart_putc(c);
        }
    }
}

/* ---- Giris noktasi ------------------------------------------------ */

void halt_forever(void)
{
    for (;;) {
        __asm__ volatile("wfe" ::: "memory");
    }
}

void kernel_panic(const char *message)
{
    uart_printf("\r\nCRITICAL_PROCESS_DIED: %s\r\n", message);
    gui_panic(message);
    halt_forever();
}

void kernel_main(void)
{
    /* Limine Base revizyonunu dogrula */
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        for (;;) {
            __asm__ volatile("wfe" ::: "memory");
        }
    }

    /* HHDM ofseti */
    if (hhdm_request.response)
        hhdm_offset = hhdm_request.response->offset;

    /* UART (PL011) seri konsol */
    uart_init(hhdm_offset + PL011_PHYS_BASE);
    uart_printf("\r\n[%s] Windows 11 Tabanli Cekirdek Onyukleniyor...\r\n", KERNEL_NAME);

    /* Framebuffer */
    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count < 1) {
        uart_puts("HATA: Framebuffer bulunamadi!\r\n");
        for (;;) {
            __asm__ volatile("wfe" ::: "memory");
        }
    }

    struct limine_framebuffer *first_fb =
        framebuffer_request.response->framebuffers[0];
    fb_init(first_fb);

    /* Bellek Haritasi */
    shell_mem();

    /* RTC Baslangic zamani */
    if (boot_time_request.response)
        boot_unix_time = boot_time_request.response->boot_time;

    timer_init();

    /* Alt sistemlerin baslatilmasi (Windows 11 Parity Stack) */
    vfs_init();
    proc_init();
    user_init();
    sec_init();
    net_init();
    sound_init();
    clipboard_init();
    notify_init();
    calculator_init();
    settings_init();
    filemgr_init();
    browser_init();
    terminal_init();
    wm_init();

    wm_create_window("Sistem Ozellikleri");
    wm_create_window("Dosya Gezgini");

    /* Masaustu grafik arayuzunu ciz */
    gui_init();
    gui_redraw_all();

    /* Ana dongu: Zamanlayici + GUI yenileme + Seri Konsol */
    uint64_t last_sec = 0;

    for (;;) {
        uint64_t freq = timer_frequency();
        uint64_t ticks = timer_ticks();
        uint64_t sec = freq ? (ticks / freq) : 0;

        if (sec != last_sec) {
            last_sec = sec;
            gui_tick(boot_unix_time, sec);
        }

        proc_tick();

        if (uart_rx_ready())
            shell_loop();
    }
}
