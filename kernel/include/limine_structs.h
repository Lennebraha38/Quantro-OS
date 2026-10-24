#ifndef LIMINE_STRUCTS_H
#define LIMINE_STRUCTS_H

#include <stdint.h>

/* Limine framebuffer structure */
struct limine_framebuffer {
    uint64_t address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};

/* Limine framebuffer response */
struct limine_framebuffer_response {
    uint64_t revision;
    uint64_t framebuffer_count;
    struct limine_framebuffer **framebuffers;
};

/* Limine memory map entry */
struct limine_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};

/* Limine memory map response */
struct limine_memmap_response {
    uint64_t revision;
    uint64_t entry_count;
    struct limine_memmap_entry *entries;
};

/* Limine file structure */
struct limine_file {
    uint64_t revision;
    char *path;
    char *cmdline;
    uint64_t address;
    uint64_t size;
};

/* Limine memmap type constants */
#define MEMMAP_USABLE              0
#define MEMMAP_RESERVED            1
#define MEMMAP_ACPI_RECLAIM        2
#define MEMMAP_ACPI_NVS            3
#define MEMMAP_BAD_MEMORY          4
#define MEMMAP_BOOTLOADER_RECLAIM  5
#define MEMMAP_KERNEL_AND_MODULES  6
#define MEMMAP_FRAMEBUFFER         7
#define MEMMAP_EFI_RECLAIMABLE     8

#endif /* LIMINE_STRUCTS_H */
