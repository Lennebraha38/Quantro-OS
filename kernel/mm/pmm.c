#include "pmm.h"
#include "uart.h"
#include "kernel.h"

#define BITMAP_SET(bm, bit)   ((bm)[(bit) / 64] |= (1ULL << ((bit) % 64)))
#define BITMAP_CLEAR(bm, bit) ((bm)[(bit) / 64] &= ~(1ULL << ((bit) % 64)))
#define BITMAP_TEST(bm, bit)  (((bm)[(bit) / 64] & (1ULL << ((bit) % 64))) != 0)

#define MAX_PHYSICAL_PAGES (1024 * 1024) /* 4 GiB max RAM icin 1M sayfa */

static uint64_t bitmap[MAX_PHYSICAL_PAGES / 64];
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t used_pages = 0;
static uintptr_t highest_address = 0;

void pmm_init(struct limine_memmap_response *memmap)
{
    if (!memmap) {
        uart_puts("[pmm] HATA: Memmap bulunamadi!\r\n");
        return;
    }

    /* Tum bitmap'i dolu (1) olarak baslat */
    memset(bitmap, 0xFF, sizeof(bitmap));

    /* Kullanilabilir en buyuk adresi bul */
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        uintptr_t top = entry->base + entry->length;
        if (top > highest_address)
            highest_address = top;
    }

    total_pages = highest_address / PAGE_SIZE;
    if (total_pages > MAX_PHYSICAL_PAGES)
        total_pages = MAX_PHYSICAL_PAGES;

    free_pages = 0;
    used_pages = 0;

    /* Sadece USABLE olan bolgeleri 0 (bos) yap */
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_page = entry->base / PAGE_SIZE;
            uint64_t page_count = entry->length / PAGE_SIZE;

            for (uint64_t p = 0; p < page_count; p++) {
                uint64_t page_idx = start_page + p;
                if (page_idx < total_pages) {
                    BITMAP_CLEAR(bitmap, page_idx);
                    free_pages++;
                }
            }
        }
    }

    /* Ilk 1MB (0x0 - 0x100000) guvenlik amaciyla dolu birak */
    uint64_t first_1mb_pages = (1024 * 1024) / PAGE_SIZE;
    for (uint64_t p = 0; p < first_1mb_pages && p < total_pages; p++) {
        if (!BITMAP_TEST(bitmap, p)) {
            BITMAP_SET(bitmap, p);
            if (free_pages > 0) free_pages--;
            used_pages++;
        }
    }

    used_pages = total_pages - free_pages;
    uart_printf("[pmm] Fiziksel Sayfa Ayirici (PMM) baslatildi: Toplam=%llu MB, Bos=%llu MB\r\n",
                (total_pages * PAGE_SIZE) / (1024 * 1024),
                (free_pages * PAGE_SIZE) / (1024 * 1024));
}

uintptr_t pmm_alloc_page(void)
{
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!BITMAP_TEST(bitmap, i)) {
            BITMAP_SET(bitmap, i);
            free_pages--;
            used_pages++;
            return (uintptr_t)(i * PAGE_SIZE);
        }
    }
    uart_puts("[pmm] HATA: Bellek Tukendi (Out of Physical Memory)!\r\n");
    return 0;
}

uintptr_t pmm_alloc_pages(size_t count)
{
    if (count == 0) return 0;
    if (count == 1) return pmm_alloc_page();

    size_t consecutive = 0;
    uint64_t start_idx = 0;

    for (uint64_t i = 0; i < total_pages; i++) {
        if (!BITMAP_TEST(bitmap, i)) {
            if (consecutive == 0) start_idx = i;
            consecutive++;
            if (consecutive == count) {
                for (uint64_t j = start_idx; j < start_idx + count; j++) {
                    BITMAP_SET(bitmap, j);
                }
                free_pages -= count;
                used_pages += count;
                return (uintptr_t)(start_idx * PAGE_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }
    return 0;
}

void pmm_free_page(uintptr_t page)
{
    uint64_t idx = page / PAGE_SIZE;
    if (idx < total_pages && BITMAP_TEST(bitmap, idx)) {
        BITMAP_CLEAR(bitmap, idx);
        free_pages++;
        if (used_pages > 0) used_pages--;
    }
}

void pmm_free_pages(uintptr_t page, size_t count)
{
    uint64_t start_idx = page / PAGE_SIZE;
    for (size_t i = 0; i < count; i++) {
        uint64_t idx = start_idx + i;
        if (idx < total_pages && BITMAP_TEST(bitmap, idx)) {
            BITMAP_CLEAR(bitmap, idx);
            free_pages++;
            if (used_pages > 0) used_pages--;
        }
    }
}

uint64_t pmm_get_total_memory(void)
{
    return total_pages * PAGE_SIZE;
}

uint64_t pmm_get_free_memory(void)
{
    return free_pages * PAGE_SIZE;
}

uint64_t pmm_get_used_memory(void)
{
    return used_pages * PAGE_SIZE;
}
