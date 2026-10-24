/*
 * Benim-OS Pro - Ana cekirdek (AArch64, Limine protokolü rev 2)
 *
 * Limine'den gelen bilgilerle framebuffer, bellek haritasi, HHDM ve
 * baslangic zamani alinir; masaustu cizimi ve seri konsol kabugu
 * baslatilir.
 */
#include "types.h"
#include "kernel.h"
#include "uart.h"
#include "string.h"
#include "fb.h"
#include "gui.h"
#include "vfs.h"
#include "net.h"
#include "sound.h"
#include "proc.h"
#include "user.h"
#include "pkg.h"
#include "sec.h"
#include "wm.h"
#include "nirvana.h"
#include "transcendence.h"
#include "omniscience.h"
#include "omnipotence.h"
#include "eternity.h"
#include "infinity.h"
#include "godmode.h"
#include "omniverse.h"
#include "simulation.h"
#include "matrix.h"
#include "reality.h"
#include "dimension.h"
#include "time.h"
#include "multiverse.h"
#include "singularity.h"
#include "consciousness.h"
#include "warp.h"
#include "fusion.h"
#include "dnastorage.h"
#include "neuromorphic.h"
#include "digitaltwin.h"
#include "predictive.h"
#include "autopatch.h"
#include "selfhealing.h"
#include "satellite.h"
#include "net5g.h"
#include "edge.h"
#include "iot.h"
#include "metaverse.h"
#include "web3.h"
#include "blockchain.h"
#include "quantum.h"
#include "ar.h"
#include "gesture.h"
#include "voice.h"
#include "biometric.h"
#include "cloudsync.h"
#include "aiassistant.h"
#include "hypervisor.h"
#include "virt.h"
#include "container.h"
#include "sandbox.h"
#include "crashreport.h"
#include "telemetry.h"
#include "l10n.h"
#include "theming.h"
#include "search.h"
#include "notify.h"
#include "clipboard.h"
#include "mgmt.h"
#include "a11y.h"
#include "ota.h"
#include "power.h"
#include "smp.h"
#include "aslr.h"
#include "fat.h"
#include "ext4.h"
#include "http.h"
#include "udp.h"
#include "tcp.h"
#include "elfloader.h"
#include "initramfs.h"
#include "blk.h"
#include "mouse.h"
#include "keyboard.h"
#include "gpu.h"
#include "browser.h"
#include "settings.h"
#include "calculator.h"
#include "filemgr.h"
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
    case LIMINE_MEMMAP_USABLE:                return "usable";
    case LIMINE_MEMMAP_RESERVED:              return "reserved";
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:      return "acpi_reclaim";
    case LIMINE_MEMMAP_ACPI_NVS:              return "acpi_nvs";
    case LIMINE_MEMMAP_BAD_MEMORY:            return "bad_memory";
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: return "reclaimable";
    case LIMINE_MEMMAP_KERNEL_AND_MODULES:    return "kernel_and_modules";
    case LIMINE_MEMMAP_FRAMEBUFFER:           return "framebuffer";
    default:                                  return "unknown";
    }
}

static void shell_mem(void)
{
    if (!memmap_request.response) {
        uart_puts("Bellek haritasi mevcut degil.\r\n");
        return;
    }

    struct limine_memmap_response *resp = memmap_request.response;

    uart_printf("Bellek giris sayisi: %u\r\n",
                (unsigned)resp->entry_count);

    for (uint64_t i = 0; i < resp->entry_count; i++) {
        struct limine_memmap_entry *e = resp->entries[i];
        uint64_t mb = e->length / (1024 * 1024);
        const char *name = memmap_type_name(e->type);

        uart_printf("[%llu] base=%llx len=%lluMiB type=%s\r\n",
                    (unsigned long long)i,
                    (unsigned long long)e->base,
                    (unsigned long long)mb,
                    name);

        if (e->type == LIMINE_MEMMAP_USABLE)
            memmap_usable_bytes += e->length;
    }

    uart_printf("Kullanilabilir toplam: %u MiB\r\n",
                (unsigned)(memmap_usable_bytes / (1024 * 1024)));
}

/* ---- UART yardimci ------------------------------------------------ */

static void uart_print_info(void)
{
    uart_printf("\r\n========================================\r\n");
    uart_printf(" %s v%s\r\n", KERNEL_NAME, KERNEL_VERSION);
    uart_printf(" Mimari: %s\r\n", KERNEL_ARCH);
    uart_printf(" Baslangic protokolu: Limine rev 2\r\n");
    if (framebuffer_request.response &&
        framebuffer_request.response->framebuffer_count > 0) {
        struct limine_framebuffer *fb =
            framebuffer_request.response->framebuffers[0];
        uart_printf(" Ekran: %ux%u (%u bpp)\r\n",
                    (unsigned)fb->width, (unsigned)fb->height,
                    (unsigned)fb->bpp);
    }
    uart_printf(" Kullanilabilir bellek: %u MiB\r\n",
                (unsigned)(memmap_usable_bytes / (1024 * 1024)));
    uart_printf(" Baslangic zamani: %lld Unix\r\n",
                (long long)boot_unix_time);
    uart_printf("========================================\r\n\r\n");
    uart_printf("benim-os> ");
}

/* ---- Kabuk -------------------------------------------------------- */

static void shell_help(void)
{
    uart_puts(
        "Komutlar:\r\n"
        "  help      - Bu ekran\r\n"
        "  ver       - Surum bilgisi\r\n"
        "  mem       - Bellek haritasi\r\n"
        "  fb        - Ekran bilgisi\r\n"
        "  uptime    - Calisma suresi\r\n"
        "  clear     - Ekrani temizle\r\n"
        "  ls        - Dosyalari listele\r\n"
        "  cat <f>  - Dosya oku\r\n"
        "  touch <f> - Dosya olustur\r\n"
        "  write <f> <data> - Dosyaya yaz\r\n"
        "  rm <f>    - Dosya sil\r\n"
        "  mv <a> <b> - Yeniden adlandir\r\n"
        "  append <f> <data> - Dosyaya ekle\r\n"
        "  cp <a> <b> - Kopyala\r\n"
        "  pwd       - Calisma dizini (root)\r\n"
        "  exists <f> - Dosya var mi?\r\n"
        "  stat <f>   - Dosya boyutu\r\n"
        "  echo <txt> - Metin yaz\r\n"
        "  halt      - Cekirdegi durdur\r\n"
        "  reboot    - Sistemi yeniden baslat (PSCI)\r\n"
        "\r\n");
}

static void shell_ver(void)
{
    uart_printf("%s v%s (%s)\r\n", KERNEL_NAME, KERNEL_VERSION, KERNEL_ARCH);
}

static void shell_fb(void)
{
    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count < 1) {
        uart_puts("Ekran bilgisi yok.\r\n");
        return;
    }

    struct limine_framebuffer *fb =
        framebuffer_request.response->framebuffers[0];

    uart_printf("Genislik:   %u\r\n", (unsigned)fb->width);
    uart_printf("Yukseklik:  %u\r\n", (unsigned)fb->height);
    uart_printf("Satir boyu: %u byte\r\n", (unsigned)fb->pitch);
    uart_printf("Renk derin.: %u bpp\r\n", (unsigned)fb->bpp);
    uart_printf("Ekran adresi: %p\r\n", (void *)(uintptr_t)fb->address);
}

static void shell_uptime(void)
{
    uint64_t ticks = timer_ticks();
    uint64_t freq = timer_frequency();
    uint64_t sec = ticks / freq;

    uart_printf("Calisma suresi: %02llu:%02llu:%02llu\r\n",
                sec / 3600, (sec / 60) % 60, sec % 60);
}

static void shell_clear(void)
{
    uart_puts("\033[2J\033[H");
}

static int psci_version(void)
{
    uint64_t version;

    __asm__ volatile("hvc #0"
                     : "=x0"(version)
                     : "x0"(0x84000000ULL), "x1"(0), "x2"(0), "x3"(0)
                     : "memory");

    return (int)version;
}

static void shell_reboot(void)
{
    uart_puts("PSCI SYSTEM_RESET gonderiliyor...\r\n");

    uint64_t ver = psci_version();
    uart_printf("PSCI surumu: %llu\r\n", (unsigned long long)ver);

    if (ver != 0)
        psci_reset();

    uart_puts("Yeniden baslatma yapilamadi.\r\n");
}

static void shell_execute(char *line)
{
    if (line[0] == '\0')
        goto prompt;

    if (strcmp(line, "help") == 0)
        shell_help();
    else if (strcmp(line, "ver") == 0)
        shell_ver();
    else if (strcmp(line, "mem") == 0)
        shell_mem();
    else if (strcmp(line, "fb") == 0)
        shell_fb();
    else if (strcmp(line, "uptime") == 0)
        shell_uptime();
    else if (strcmp(line, "clear") == 0)
        shell_clear();
    else if (strcmp(line, "halt") == 0) {
        uart_puts("Durduruluyor...\r\n");
        halt_forever();
    } else if (strcmp(line, "reboot") == 0)
        shell_reboot();
    else
        uart_printf("Bilinmeyen komut: %s\r\n", line);

prompt:
    uart_printf("benim-os> ");
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
    uart_printf("\r\nPANIC: %s\r\n", message);
    gui_panic(message);
    halt_forever();
}

void kernel_main(void)
{
    /* Base revizyonu dogrula */
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        for (;;) {
            ;
        }
    }

    /* HHDM (yuksek yarimayfa esleme ofseti) */
    if (hhdm_request.response)
        hhdm_offset = hhdm_request.response->offset;

    /* PL011 UART'i baslat (debug / seri konsol) */
    uart_init(hhdm_offset + PL011_PHYS_BASE);

    uart_printf("\r\n[%s] Cekirdek calismaya basladi.\r\n", KERNEL_NAME);

    /* Framebuffer */
    if (!framebuffer_request.response ||
        framebuffer_request.response->framebuffer_count < 1) {
        uart_puts("HATA: framebuffer yok; GUI devre disi.\r\n");
        for (;;) {
            __asm__ volatile("wfe" ::: "memory");
        }
    }

    struct limine_framebuffer *first_fb =
        framebuffer_request.response->framebuffers[0];
    fb_init(first_fb);

    /* Bellek haritasi taramasi */
    shell_mem();

    /* Baslangic zamani (sistem RTC'sinden) */
    if (boot_time_request.response)
        boot_unix_time = boot_time_request.response->boot_time;

    timer_init();

    /* Alt sistemler */
    vfs_init();
    net_init();
    sound_init();
    proc_init();
    user_init();
    pkg_init();
    sec_init();
    wm_init();
    nirvana_init();
    transcendence_init();
    omniscience_init();
    omnipotence_init();
    eternity_init();
    infinity_init();
    godmode_init();
    omniverse_init();
    simulation_init();
    matrix_init();
    reality_init();
    dimension_init();
    time_init();
    multiverse_init();
    singularity_init();
    consciousness_init();
    warp_init();
    fusion_init();
    dnastorage_init();
    neuromorphic_init();
    digitaltwin_init();
    predictive_init();
    autopatch_init();
    selfhealing_init();
    satellite_init();
    net5g_init();
    edge_init();
    iot_init();
    metaverse_init();
    web3_init();
    blockchain_init();
    quantum_init();
    ar_init();
    gesture_init();
    voice_init();
    biometric_init();
    cloudsync_init();
    aiassistant_init();
    hypervisor_init();
    virt_init();
    container_init();
    sandbox_init();
    crashreport_init();
    telemetry_init();
    l10n_init();
    theming_init();
    search_init();
    notify_init();
    clipboard_init();
    mgmt_init();
    a11y_init();
    ota_init();
    power_init();
    smp_init();
    aslr_init();
    fat_init();
    ext4_init();
    http_init();
    udp_init();
    tcp_init();
    elfloader_init();
    initramfs_init();
    blk_init();
    mouse_init();
    keyboard_init();
    gpu_init();
    wm_create_window("Quantro Desktop");
    browser_init();
    settings_init();
    calculator_init();
    filemgr_init();
    terminal_init();

    /* Masaustu arayuzu */
    gui_init();
    gui_redraw_all();

    /* Ana dongu: saniyelik guncelleme + UART kabugu */
    uint64_t last_sec = 0;

    for (;;) {
        uint64_t freq = timer_frequency();
        uint64_t ticks = timer_ticks();
        uint64_t sec = ticks / freq;

        if (sec != last_sec) {
            last_sec = sec;
            gui_tick((uint64_t)boot_unix_time, sec);
        }

        proc_tick();

        if (uart_rx_ready())
            shell_loop();
    }
}
