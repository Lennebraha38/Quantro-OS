#ifndef QUANTRO_VMM_H
#define QUANTRO_VMM_H

#include "types.h"

#define VMM_FLAG_READ    (1 << 0)
#define VMM_FLAG_WRITE   (1 << 1)
#define VMM_FLAG_EXEC    (1 << 2)
#define VMM_FLAG_USER    (1 << 3)
#define VMM_FLAG_NOCACHE (1 << 4)

/* AArch64 MMU Page Entry Bits */
#define PTE_VALID       (1ULL << 0)
#define PTE_PAGE        (1ULL << 1)
#define PTE_TABLE       (1ULL << 1)
#define PTE_ATTRINDX(x) (((uint64_t)(x) & 0x7) << 2)
#define PTE_NON_SECURE  (1ULL << 5)
#define PTE_AP_RW_EL1   (0ULL << 6)
#define PTE_AP_RW_ALL   (1ULL << 6)
#define PTE_AP_RO_EL1   (2ULL << 6)
#define PTE_AP_RO_ALL   (3ULL << 6)
#define PTE_SH_INNER    (3ULL << 8)
#define PTE_AF          (1ULL << 10)
#define PTE_PXN         (1ULL << 53)
#define PTE_UXN         (1ULL << 54)

void     vmm_init(uintptr_t hhdm);
int      vmm_map_page(uint64_t *table, uintptr_t vaddr, uintptr_t paddr, uint64_t flags);
int      vmm_unmap_page(uint64_t *table, uintptr_t vaddr);
uintptr_t vmm_virt_to_phys(uint64_t *table, uintptr_t vaddr);
uint64_t *vmm_get_kernel_table(void);

#endif /* QUANTRO_VMM_H */
