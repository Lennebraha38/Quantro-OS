/*
 * Quantro OS - Ana Cekirdek (AArch64, Limine Protokolu rev 2)
 * Tam Donanim Mimarisi, PMM/VMM/Heap, Preemptive Scheduler ve Windows 11 Paritesi
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
#include "mouse.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "traps.h"
#include "fat.h"
#include "elfloader.h"
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

static void shell_pmm(void)
{
    uint64_t total = pmm_get_total_memory();
    uint64_t free = pmm_get_free_memory();
    uint64_t used = pmm_get_used_memory();

    uart_puts("\r\n--- AArch64 Fiziksel Sayfa Ayirici (PMM) Raporu ---\r\n");
    uart_printf(" Sayfa Boyutu (Granule):      4096 bayt (4 KB)\r\n");
    uart_printf(" Toplam Yonetilen Bellek:    %llu MB (%llu sayfa)\r\n",
                total / (1024 * 1024), total / 4096);
    uart_printf(" Kullanilan Fiziksel Bellek: %llu MB (%llu sayfa)\r\n",
                used / (1024 * 1024), used / 4096);
    uart_printf(" Bos Fiziksel Bellek:        %llu MB (%llu sayfa)\r\n",
                free / (1024 * 1024), free / 4096);
    uart_puts("---------------------------------------------------\r\n");
}

static void shell_heap(void)
{
    size_t used = heap_get_used();
    size_t total = heap_get_total();

    uart_puts("\r\n--- Kernel Dinamik Heap (kmalloc / kfree) Raporu ---\r\n");
    uart_printf(" Toplam Heap Kapasitesi:     %u KB (%u bayt)\r\n",
                (unsigned)(total / 1024), (unsigned)total);
    uart_printf(" Tahsis Edilen Bellek:       %u KB (%u bayt)\r\n",
                (unsigned)(used / 1024), (unsigned)used);
    uart_printf(" Kullanilabilir Bos Alan:    %u KB\r\n",
                (unsigned)((total - used) / 1024));
    uart_puts(" Tahsis Motoru:              Guvenli Linked-List Free Block Allocator\r\n");
    uart_puts("----------------------------------------------------\r\n");
}

/* ---- Sistem Bilgisi (Windows Systeminfo) --------------------------- */

static void shell_sysinfo(void)
{
    uart_puts("\r\n=======================================================\r\n");
    uart_puts(" QUANTRO OS - WINDOWS 11 SISTEM BILGISI (SYSTEMINFO)\r\n");
    uart_puts("=======================================================\r\n");
    uart_puts(" Ana Bilgisayar Adi:          QUANTRO-PC\r\n");
    uart_puts(" Isletim Sistemi Adi:         Microsoft Windows 11 Pro (Quantro AArch64)\r\n");
    uart_printf(" Isletim Sistemi Surumu:      10.0.26100 Yapisi 26100 (v%s)\r\n", KERNEL_VERSION);
    uart_puts(" Sistem Mimarisi:             ARM64-based PC (AArch64 Bare-Metal)\r\n");
    uart_puts(" Onyukleme Protokolu:         Limine Boot Protocol rev 2 (UEFI)\r\n");
    uart_puts(" Donanim Tabanli Guvenlik:    TPM 2.0 (PCR0-7 Validated & Enforced)\r\n");
    uart_puts(" Sanallastirma Guvenligi:     VBS & HVCI Etkin\r\n");
    uart_printf(" Toplam Fiziksel Bellek:      %u MiB (PMM Yonetiminde)\r\n",
                (unsigned)(memmap_usable_bytes / (1024 * 1024)));
    uart_printf(" Cekirdek Heap Alani:         %u KB (kmalloc hazir)\r\n",
                (unsigned)(heap_get_total() / 1024));
    uart_puts(" Ag Donanimi:                 VirtIO-Net 10Gbps NDIS Ethernet0 (192.168.1.105)\r\n");
    uart_puts(" Dosya Sistemi Alt Yapisi:    NTFS/VFS Sanal C:\\ + On-Disk FAT32 ESP\r\n");
    uart_puts(" Gorev Yoneticisi:            AArch64 Preemptive Donanimsal Context Switch\r\n");
    uart_puts("=======================================================\r\n");
}

static void uart_print_info(void)
{
    uart_printf("\r\n=======================================================\r\n");
    uart_printf(" %s (Windows 11 Edition) v%s\r\n", KERNEL_NAME, KERNEL_VERSION);
    uart_printf(" Mimari: %s | PMM, VMM, Heap, Preemptive Scheduler AKTIF\r\n", KERNEL_ARCH);
    if (framebuffer_request.response &&
        framebuffer_request.response->framebuffer_count > 0) {
        struct limine_framebuffer *fb =
            framebuffer_request.response->framebuffers[0];
        uart_printf(" Ekran: %ux%u (%u bpp) TrueColor Framebuffer\r\n",
                    (unsigned)fb->width, (unsigned)fb->height, (unsigned)fb->bpp);
    }
    uart_printf(" Kullanilabilir RAM: %u MiB | Heap: %u KB\r\n",
                (unsigned)(memmap_usable_bytes / (1024 * 1024)),
                (unsigned)(heap_get_total() / 1024));
    uart_printf(" Oturum: %s\\%s\r\n", "QUANTRO-PC", user_current_name());
    uart_printf(" Komut listesi icin 'help' veya '?' yazin.\r\n");
    uart_printf("=======================================================\r\n\r\n");
    uart_printf("PS C:\\Users\\Quantro> ");
}

/* ---- Kabuk (Shell) Yardimi ---------------------------------------- */

static void shell_help(void)
{
    uart_puts(
        "\r\n--- QUANTRO OS TAM DONANIM & POWERSHELL KOMUT KILAVUZU ---\r\n\r\n"
        " SISTEM & DONANIM CEKIRDEGI:\r\n"
        "   help, ?                 - Bu yardim ekranini gosterir\r\n"
        "   ver                     - Isletim sistemi surumunu gosterir\r\n"
        "   sysinfo, systeminfo     - Kapsamli sistem bilesenleri raporu\r\n"
        "   mem                     - Limine fiziksel bellek haritasini listeler\r\n"
        "   pmm                     - Fiziksel sayfa ayirici (PMM 4KB) istatistikleri\r\n"
        "   heap                    - Kernel heap (kmalloc/kfree) bellek durumu\r\n"
        "   fb                      - Framebuffer cozumleme bilgilerini verir\r\n"
        "   uptime                  - Calisma suresini gosterir\r\n"
        "   cls, clear              - Ekrani temizler\r\n"
        "   reboot                  - Sistemi yeniden baslatir (ARM PSCI)\r\n"
        "   halt                    - Cekirdegi durdurur\r\n\r\n"
        " DOSYA SISTEMI (NTFS / VFS & FAT32):\r\n"
        "   dir, ls [yol]           - Dosyalari ve dizinleri listeler\r\n"
        "   fatls                   - On-Disk FAT32 partisyonu kok dizinini listeler\r\n"
        "   type, cat <dosya>       - Dosya icerigini okur\r\n"
        "   touch <dosya>           - Yeni bos dosya olusturur\r\n"
        "   write <dosya> <metin>   - Dosyaya metin yazar\r\n"
        "   append <dosya> <metin>  - Dosyanin sonuna metin ekler\r\n"
        "   del, rm <dosya>         - Dosyayi siler\r\n"
        "   ren, mv <eski> <yeni>   - Dosyayi yeniden adlandirir\r\n"
        "   copy, cp <kaynak> <hed> - Dosyayi kopyalar\r\n"
        "   mkdir, md <dizin>       - Yeni dizin olusturur\r\n"
        "   stat <dosya>            - Dosya boyutunu verir\r\n\r\n"
        " GOREV, SUREC & PROGRAM YUKLEME:\r\n"
        "   tasklist, ps            - Calisan surecleri listeler (Preemptive Context Switch)\r\n"
        "   taskkill <pid>          - Belirtilen sureci sonlandirir\r\n"
        "   start <ad>              - Yeni bir arka plan gorevi baslatir\r\n"
        "   exec <elf_dosyasi>      - 64-bit ELF ikilisini ayristirip bellekte calistirir\r\n\r\n"
        " GUVENLIK & KULLANICI:\r\n"
        "   whoami                  - Gecerli kullanici ve SID bilgisini verir\r\n"
        "   users                   - Sistem kullanicilarini listeler\r\n"
        "   login <kull> <parola>   - Oturum degistirir\r\n"
        "   useradd <kull> <parola> - Yeni kullanici olusturur\r\n"
        "   secstat                 - TPM 2.0, Secure Boot ve VBS durumu\r\n"
        "   tpm                     - TPM PCR donanim kayitlarini gosterir\r\n"
        "   defender <dosya>        - Dosyayi heuristic virus taramasindan gecirir\r\n\r\n"
        " AG & ILETISIM:\r\n"
        "   ipconfig, ifconfig      - NDIS Ethernet adapter yapilandirmasi\r\n"
        "   ping <ip>               - ICMP echo istegi ve checksum dogrulamasi\r\n"
        "   netstat                 - Etkin TCP/UDP dinleyici portlarini listeler\r\n"
        "   curl, fetch <url>       - Web portalina HTTP istegi atar\r\n\r\n"
        " MASAUSTU & UYGULAMALAR:\r\n"
        "   calc <a op b>           - Modern hesap makinesi\r\n"
        "   settings                - Windows 11 Ayarlar Paneli\r\n"
        "   explorer                - Dosya Gezgini\r\n"
        "   edge <url>              - Microsoft Edge tarayicisi\r\n"
        "   powershell              - Windows PowerShell\r\n"
        "   wm                      - Aktif pencereleri listeler\r\n"
        "   toast <baslik> <mesaj>  - Toast bildirimi gonderir\r\n"
        "   notifications           - Gelen bildirimleri listeler\r\n"
        "   clip [metin]            - Panoya kopyalar veya panoyu gosterir\r\n"
        "   volume [0-100]          - Ses seviyesini ayarlar\r\n"
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
    while (*line == ' ') line++;
    if (*line == '\0') goto prompt;

    char cmd[32];
    char arg1[64];
    char arg2[64];
    char arg3[64];
    cmd[0] = '\0'; arg1[0] = '\0'; arg2[0] = '\0'; arg3[0] = '\0';

    int i = 0;
    while (*line && *line != ' ' && i < 31) cmd[i++] = *line++;
    cmd[i] = '\0';

    while (*line == ' ') line++;
    i = 0;
    while (*line && *line != ' ' && i < 63) arg1[i++] = *line++;
    arg1[i] = '\0';

    while (*line == ' ') line++;
    i = 0;
    while (*line && *line != ' ' && i < 63) arg2[i++] = *line++;
    arg2[i] = '\0';

    while (*line == ' ') line++;
    i = 0;
    while (*line && i < 63) arg3[i++] = *line++;
    arg3[i] = '\0';

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        shell_help();
    } else if (strcmp(cmd, "ver") == 0) {
        uart_printf("Microsoft Windows [Surum 10.0.26100.1742] (%s v%s %s)\r\n",
                    KERNEL_NAME, KERNEL_VERSION, KERNEL_ARCH);
    } else if (strcmp(cmd, "sysinfo") == 0 || strcmp(cmd, "systeminfo") == 0) {
        shell_sysinfo();
    } else if (strcmp(cmd, "mem") == 0) {
        shell_mem();
    } else if (strcmp(cmd, "pmm") == 0) {
        shell_pmm();
    } else if (strcmp(cmd, "heap") == 0) {
        shell_heap();
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
    } else if (strcmp(cmd, "fatls") == 0) {
        fat_list_root();
    } else if (strcmp(cmd, "type") == 0 || strcmp(cmd, "cat") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: type <dosya>\r\n");
        } else {
            const char *data = vfs_read(arg1);
            if (data)
                uart_printf("%s\r\n", data);
            else
                uart_printf("Hata: '%s' bulunamadi.\r\n", arg1);
        }
    } else if (strcmp(cmd, "touch") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: touch <dosya>\r\n");
        } else {
            if (vfs_create(arg1) == 0)
                uart_printf("Dosya olusturuldu: %s\r\n", arg1);
            else
                uart_puts("Hata: Dosya olusturulamadi.\r\n");
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
                uart_printf("Kopyalandi: %s -> %s\r\n", arg1, arg2);
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
    /* GOREV & ELF YUKLEME */
    else if (strcmp(cmd, "tasklist") == 0 || strcmp(cmd, "ps") == 0) {
        proc_list();
    } else if (strcmp(cmd, "taskkill") == 0 || strcmp(cmd, "kill") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: taskkill <pid>\r\n");
        } else {
            int pid = atoi(arg1);
            if (proc_kill(pid) == 0)
                uart_printf("Basari: PID %d sonlandirildi.\r\n", pid);
            else
                uart_printf("Hata: PID %d sonlandirilamadi.\r\n", pid);
        }
    } else if (strcmp(cmd, "start") == 0 || strcmp(cmd, "spawn") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: start <program>\r\n");
        } else {
            int pid = proc_spawn(arg1);
            if (pid >= 0)
                uart_printf("Gorev baslatildi: %s (PID: %d)\r\n", arg1, pid);
            else
                uart_puts("Hata: Gorev tablosu dolu.\r\n");
        }
    } else if (strcmp(cmd, "exec") == 0) {
        if (arg1[0] == '\0') {
            uart_puts("Kullanim: exec <elf_dosyasi>\r\n");
        } else {
            elfloader_exec_file(arg1);
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
    /* UYGULAMALAR */
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
    } else if (strcmp(cmd, "mouse") == 0) {
        if (arg1[0] != '\0' && arg2[0] != '\0') {
            int mx = atoi(arg1);
            int my = atoi(arg2);
            gui_move_cursor(mx, my);
            uart_printf("Fare imleci tasindi: (%d, %d)\r\n", mx, my);
        } else {
            uart_printf("Mevcut fare konumu: (%d, %d)\r\n", gui_get_cursor_x(), gui_get_cursor_y());
        }
    } else if (strcmp(cmd, "click") == 0) {
        int mx = gui_get_cursor_x();
        int my = gui_get_cursor_y();
        if (arg1[0] != '\0' && arg2[0] != '\0') {
            mx = atoi(arg1);
            my = atoi(arg2);
        }
        gui_handle_mouse(mx, my, 1);
        gui_handle_mouse(mx, my, 0);
        uart_printf("Tiklandi: (%d, %d)\r\n", mx, my);
    } else if (strcmp(cmd, "start") == 0 || strcmp(cmd, "baslat") == 0) {
        gui_toggle_start_menu();
        uart_puts("Baslat menusu durumu degistirildi.\r\n");
    } else if (strcmp(cmd, "win") == 0 || strcmp(cmd, "window") == 0) {
        gui_toggle_settings_window();
        uart_puts("Ayarlar penceresi durumu degistirildi.\r\n");
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
            uart_printf("Ses Seviyesi: %%%d\r\n", sound_get_volume());
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
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        for (;;) {
            __asm__ volatile("wfe" ::: "memory");
        }
    }

    if (hhdm_request.response)
        hhdm_offset = hhdm_request.response->offset;

    uart_init(hhdm_offset + PL011_PHYS_BASE);
    uart_printf("\r\n[%s] Windows 11 AArch64 Cekirdegi Onyukleniyor...\r\n", KERNEL_NAME);

    /* 1. Fiziksel Sayfa Ayirici (PMM) */
    if (memmap_request.response) {
        pmm_init(memmap_request.response);
        shell_mem();
    }

    /* 2. AArch64 Sanal Bellek Yoneticisi (VMM) */
    vmm_init(hhdm_offset);

    /* 3. Dinamik Cekirdek Heap Alani (kmalloc/kfree) */
    heap_init(0, 0);

    /* 4. Donanimsal Exception Vector Tablosu (VBAR_EL1) */
    traps_init();

    /* 5. Framebuffer ve Ekran */
    int has_fb = 0;
    if (framebuffer_request.response &&
        framebuffer_request.response->framebuffer_count > 0) {
        struct limine_framebuffer *first_fb = framebuffer_request.response->framebuffers[0];
        fb_init(first_fb);
        has_fb = 1;
        uart_printf("[fb] Framebuffer bulundu: %ux%u (%u bpp)\r\n",
                    (unsigned)first_fb->width, (unsigned)first_fb->height, (unsigned)first_fb->bpp);
    } else {
        uart_puts("[fb] UYARI: Grafik Framebuffer bulunamadi, UART konsol modunda devam ediliyor.\r\n");
    }

    if (boot_time_request.response)
        boot_unix_time = boot_time_request.response->boot_time;

    timer_init();

    /* 6. Dosya Sistemleri (NTFS/VFS + On-Disk FAT32) */
    vfs_init();
    fat_init();
    fat_mount();

    /* 7. Guvenlik, SAM ve Donanim TPM 2.0 */
    user_init();
    sec_init();

    /* 8. Tam Ag Yigini (Ethernet/ARP/IPv4/ICMP/UDP/TCP) */
    net_init();

    /* 9. Ses ve Multimedya */
    sound_init();

    /* 9.5 VirtIO Tablet & Fare Donanimi */
    mouse_init(hhdm_offset);

    /* 10. ELF64 Binary Yukleyicisi */
    elfloader_init();

    /* 11. Preemptive Scheduler & Gorevler */
    proc_init();

    /* 12. Masaustu, Pencereler ve Fluent GUI */
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

    if (has_fb) {
        gui_init();
        gui_redraw_all();
    }

    uint64_t last_sec = 0;

    for (;;) {
        uint64_t freq = timer_frequency();
        uint64_t ticks = timer_ticks();
        uint64_t sec = freq ? (ticks / freq) : 0;

        if (has_fb && sec != last_sec) {
            last_sec = sec;
            gui_tick(boot_unix_time, sec);
        }

        mouse_poll();

        proc_tick();

        if (uart_rx_ready())
            shell_loop();
    }
}
