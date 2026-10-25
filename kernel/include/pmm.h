#ifndef QUANTRO_PMM_H
#define QUANTRO_PMM_H

#include "types.h"
#include "limine.h"

#define PAGE_SIZE 4096ULL

void     pmm_init(struct limine_memmap_response *memmap);
uintptr_t pmm_alloc_page(void);
uintptr_t pmm_alloc_pages(size_t count);
void     pmm_free_page(uintptr_t page);
void     pmm_free_pages(uintptr_t page, size_t count);
uint64_t pmm_get_total_memory(void);
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_used_memory(void);

#endif /* QUANTRO_PMM_H */
