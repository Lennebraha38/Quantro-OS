#include "fat.h"
#include "uart.h"
#include "kernel.h"

#define FAT_DISK_SECTOR_SIZE 512
#define FAT_MAX_OPEN_FILES   8

static fat32_bpb_t g_bpb;
static int g_mounted = 0;
static uint32_t g_data_start_lba = 0;

/* Sentetik FAT32 Disk Imgesi (ESP / Boot bolumu simulasyonu) */
static uint8_t simulated_fat32_image[32 * 1024] __attribute__((aligned(512)));

typedef struct {
    int      used;
    uint32_t current_cluster;
    uint32_t file_size;
    uint32_t offset;
    char     name[16];
} fat_file_desc_t;

static fat_file_desc_t open_files[FAT_MAX_OPEN_FILES];

void fat_init(void)
{
    memset(simulated_fat32_image, 0, sizeof(simulated_fat32_image));

    /* Gercek FAT32 BPB Basligini olustur */
    fat32_bpb_t *bpb = (fat32_bpb_t *)simulated_fat32_image;
    bpb->bootjmp[0] = 0xEB; bpb->bootjmp[1] = 0x58; bpb->bootjmp[2] = 0x90;
    memcpy(bpb->oem_name, "MSWIN4.1", 8);
    bpb->bytes_per_sector = 512;
    bpb->sectors_per_cluster = 1;
    bpb->reserved_sectors = 32;
    bpb->fat_count = 2;
    bpb->total_sectors_32 = 64; /* 32 KB disk */
    bpb->fat_size_32 = 4;
    bpb->root_cluster = 2;
    memcpy(bpb->volume_label, "QUANTRO_ESP", 11);
    memcpy(bpb->fs_type, "FAT32   ", 8);

    /* 0x55AA MBR/VBR Imzasi */
    simulated_fat32_image[510] = 0x55;
    simulated_fat32_image[511] = 0xAA;

    /* Cluster 2 (Kök Dizin) girislerini yaz */
    uint32_t root_lba = 32 + (2 * 4); /* reserved + fats */
    fat32_dir_entry_t *root = (fat32_dir_entry_t *)(simulated_fat32_image + (root_lba * 512));

    /* 1. Giris: KERNEL.ELF */
    memcpy(root[0].name, "KERNEL  ELF", 11);
    root[0].attr = 0x20; /* Archive */
    root[0].cluster_low = 3;
    root[0].file_size = 1024;

    /* 2. Giris: LIMINE  CFG */
    memcpy(root[1].name, "LIMINE  CFG", 11);
    root[1].attr = 0x20;
    root[1].cluster_low = 4;
    root[1].file_size = 156;

    /* Cluster 4 verisi: limine.conf */
    uint32_t cfg_lba = root_lba + 2;
    char *cfg_data = (char *)(simulated_fat32_image + (cfg_lba * 512));
    strncpy(cfg_data, "TIMEOUT=3\r\n:Quantro OS\r\nPROTOCOL=limine\r\nKERNEL_PATH=boot:///kernel\r\n", 511);

    for (int i = 0; i < FAT_MAX_OPEN_FILES; i++)
        open_files[i].used = 0;

    uart_puts("[fs:fat] FAT32 On-Disk Sektor Ayristiricisi hazir (ESP Partisyonu)\r\n");
}

int fat_mount(void)
{
    fat32_bpb_t *bpb = (fat32_bpb_t *)simulated_fat32_image;

    if (simulated_fat32_image[510] != 0x55 || simulated_fat32_image[511] != 0xAA) {
        uart_puts("[fs:fat] HATA: Gecersiz FAT32 boot sektoru (Imza 0x55AA yok)!\r\n");
        return -1;
    }

    memcpy(&g_bpb, bpb, sizeof(fat32_bpb_t));
    g_data_start_lba = g_bpb.reserved_sectors + (g_bpb.fat_count * g_bpb.fat_size_32);
    g_mounted = 1;

    uart_printf("[fs:fat] FAT32 Bolumu Baglandi (Mount): OEM='%.8s', ClusterSize=%u B, RootCluster=%u\r\n",
                g_bpb.oem_name,
                g_bpb.bytes_per_sector * g_bpb.sectors_per_cluster,
                g_bpb.root_cluster);
    return 0;
}

void fat_list_root(void)
{
    if (!g_mounted) fat_mount();

    uart_puts("\r\n FAT32 Kök Dizin Listesi (/mnt/fat):\r\n");
    uart_puts(" ----------------------------------------\r\n");

    uint32_t root_lba = g_data_start_lba;
    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)(simulated_fat32_image + (root_lba * 512));

    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00) break;   /* Bitti */
        if (entries[i].name[0] == 0xE5) continue; /* Silinmis */

        char fname[13];
        int k = 0;
        for (int j = 0; j < 8 && entries[i].name[j] != ' '; j++)
            fname[k++] = entries[i].name[j];
        if (entries[i].name[8] != ' ') {
            fname[k++] = '.';
            for (int j = 8; j < 11 && entries[i].name[j] != ' '; j++)
                fname[k++] = entries[i].name[j];
        }
        fname[k] = '\0';

        uart_printf("  %-14s  Cluster: %3u  Boyut: %u bayt\r\n",
                    fname,
                    entries[i].cluster_low,
                    entries[i].file_size);
    }
}

int fat_open(const char *path)
{
    if (!g_mounted) fat_mount();

    uint32_t root_lba = g_data_start_lba;
    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)(simulated_fat32_image + (root_lba * 512));

    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00) break;
        if (entries[i].name[0] == 0xE5) continue;

        char fname[13];
        int k = 0;
        for (int j = 0; j < 8 && entries[i].name[j] != ' '; j++)
            fname[k++] = entries[i].name[j];
        if (entries[i].name[8] != ' ') {
            fname[k++] = '.';
            for (int j = 8; j < 11 && entries[i].name[j] != ' '; j++)
                fname[k++] = entries[i].name[j];
        }
        fname[k] = '\0';

        if (strcasecmp(fname, path) == 0) {
            for (int fd = 0; fd < FAT_MAX_OPEN_FILES; fd++) {
                if (!open_files[fd].used) {
                    open_files[fd].used = 1;
                    open_files[fd].current_cluster = entries[i].cluster_low;
                    open_files[fd].file_size = entries[i].file_size;
                    open_files[fd].offset = 0;
                    strncpy(open_files[fd].name, fname, 15);
                    return fd;
                }
            }
        }
    }
    return -1;
}

int fat_read(int fd, void *buf, uint32_t len)
{
    if (fd < 0 || fd >= FAT_MAX_OPEN_FILES || !open_files[fd].used)
        return -1;

    uint32_t avail = open_files[fd].file_size - open_files[fd].offset;
    if (len > avail) len = avail;
    if (len == 0) return 0;

    uint32_t cluster = open_files[fd].current_cluster;
    uint32_t lba = g_data_start_lba + (cluster - 2) * g_bpb.sectors_per_cluster;
    uint8_t *src = simulated_fat32_image + (lba * 512) + open_files[fd].offset;

    memcpy(buf, src, len);
    open_files[fd].offset += len;

    return (int)len;
}
