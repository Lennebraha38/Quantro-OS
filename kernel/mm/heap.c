#include "heap.h"
#include "uart.h"
#include "kernel.h"

#define HEAP_MAGIC 0x48454150 /* 'HEAP' */
#define HEAP_ALIGN 16

typedef struct heap_block {
    uint32_t magic;
    uint8_t  is_free;
    uint8_t  reserved[3];
    size_t   size;
    struct heap_block *next;
} heap_block_t;

#define BLOCK_HEADER_SIZE ((sizeof(heap_block_t) + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1))

#define STATIC_HEAP_SIZE (2 * 1024 * 1024) /* 2 MB Kernel Heap */
static uint8_t g_heap_memory[STATIC_HEAP_SIZE] __attribute__((aligned(16)));

static heap_block_t *head_block = 0;
static size_t total_heap_size = 0;
static size_t used_heap_size = 0;

void heap_init(uintptr_t base_vaddr, size_t initial_size)
{
    (void)base_vaddr;
    (void)initial_size;

    head_block = (heap_block_t *)g_heap_memory;
    head_block->magic = HEAP_MAGIC;
    head_block->is_free = 1;
    head_block->size = STATIC_HEAP_SIZE - BLOCK_HEADER_SIZE;
    head_block->next = 0;

    total_heap_size = STATIC_HEAP_SIZE;
    used_heap_size = BLOCK_HEADER_SIZE;

    uart_printf("[heap] Dynamic Kernel Heap (kmalloc/kfree) baslatildi: Boyut=%u KB\r\n",
                (unsigned)(STATIC_HEAP_SIZE / 1024));
}

void *kmalloc(size_t size)
{
    if (size == 0) return 0;

    size = (size + (HEAP_ALIGN - 1)) & ~(HEAP_ALIGN - 1);

    heap_block_t *curr = head_block;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            /* Bolme kontrolu */
            if (curr->size >= size + BLOCK_HEADER_SIZE + HEAP_ALIGN) {
                heap_block_t *new_block = (heap_block_t *)((uint8_t *)curr + BLOCK_HEADER_SIZE + size);
                new_block->magic = HEAP_MAGIC;
                new_block->is_free = 1;
                new_block->size = curr->size - size - BLOCK_HEADER_SIZE;
                new_block->next = curr->next;

                curr->size = size;
                curr->next = new_block;
            }

            curr->is_free = 0;
            used_heap_size += curr->size + BLOCK_HEADER_SIZE;
            return (void *)((uint8_t *)curr + BLOCK_HEADER_SIZE);
        }
        curr = curr->next;
    }

    uart_printf("[heap] HATA: kmalloc(%u) basarisiz, yer kalmadi!\r\n", (unsigned)size);
    return 0;
}

void *kzalloc(size_t size)
{
    void *ptr = kmalloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void kfree(void *ptr)
{
    if (!ptr) return;

    heap_block_t *blk = (heap_block_t *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);
    if (blk->magic != HEAP_MAGIC) {
        uart_puts("[heap] HATA: Gecersiz kfree adresi / bozulmus header!\r\n");
        return;
    }

    blk->is_free = 1;
    if (used_heap_size >= blk->size + BLOCK_HEADER_SIZE)
        used_heap_size -= (blk->size + BLOCK_HEADER_SIZE);

    /* Ardisik serbest bloklari birlestir (Coalesce) */
    heap_block_t *curr = head_block;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += curr->next->size + BLOCK_HEADER_SIZE;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

size_t heap_get_used(void)
{
    return used_heap_size;
}

size_t heap_get_total(void)
{
    return total_heap_size;
}
