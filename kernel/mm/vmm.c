#include "vmm.h"
#include "pmm.h"
#include "uart.h"
#include "kernel.h"

static uintptr_t g_hhdm = 0;
static uint64_t *kernel_l0_table = 0;

static inline uintptr_t phys_to_virt(uintptr_t phys)
{
    return phys + g_hhdm;
}

static inline uintptr_t virt_to_phys_direct(uintptr_t virt)
{
    if (virt >= g_hhdm)
        return virt - g_hhdm;
    return virt;
}

void vmm_init(uintptr_t hhdm)
{
    g_hhdm = hhdm;

    /* Çekirdek L0 ana sayfa tablosunu tahsis et */
    uintptr_t l0_phys = pmm_alloc_page();
    if (!l0_phys) {
        uart_puts("[vmm] HATA: L0 sayfa tablosu tahsis edilemedi!\r\n");
        return;
    }

    kernel_l0_table = (uint64_t *)phys_to_virt(l0_phys);
    memset(kernel_l0_table, 0, PAGE_SIZE);

    uart_printf("[vmm] AArch64 4-Level Sanal Bellek Yoneticisi (VMM) hazir: L0 Table=0x%llx\r\n",
                (unsigned long long)l0_phys);
}

uint64_t *vmm_get_kernel_table(void)
{
    return kernel_l0_table;
}

int vmm_map_page(uint64_t *l0_table, uintptr_t vaddr, uintptr_t paddr, uint64_t flags)
{
    if (!l0_table) return -1;

    size_t l0_idx = (vaddr >> 39) & 0x1FF;
    size_t l1_idx = (vaddr >> 30) & 0x1FF;
    size_t l2_idx = (vaddr >> 21) & 0x1FF;
    size_t l3_idx = (vaddr >> 12) & 0x1FF;

    /* Seviye 0 -> Seviye 1 */
    uint64_t *l1_table;
    if (!(l0_table[l0_idx] & PTE_VALID)) {
        uintptr_t p = pmm_alloc_page();
        if (!p) return -1;
        l1_table = (uint64_t *)phys_to_virt(p);
        memset(l1_table, 0, PAGE_SIZE);
        l0_table[l0_idx] = p | PTE_VALID | PTE_TABLE | PTE_AF;
    } else {
        uintptr_t p = l0_table[l0_idx] & 0x0000FFFFFFFFF000ULL;
        l1_table = (uint64_t *)phys_to_virt(p);
    }

    /* Seviye 1 -> Seviye 2 */
    uint64_t *l2_table;
    if (!(l1_table[l1_idx] & PTE_VALID)) {
        uintptr_t p = pmm_alloc_page();
        if (!p) return -1;
        l2_table = (uint64_t *)phys_to_virt(p);
        memset(l2_table, 0, PAGE_SIZE);
        l1_table[l1_idx] = p | PTE_VALID | PTE_TABLE | PTE_AF;
    } else {
        uintptr_t p = l1_table[l1_idx] & 0x0000FFFFFFFFF000ULL;
        l2_table = (uint64_t *)phys_to_virt(p);
    }

    /* Seviye 2 -> Seviye 3 */
    uint64_t *l3_table;
    if (!(l2_table[l2_idx] & PTE_VALID)) {
        uintptr_t p = pmm_alloc_page();
        if (!p) return -1;
        l3_table = (uint64_t *)phys_to_virt(p);
        memset(l3_table, 0, PAGE_SIZE);
        l2_table[l2_idx] = p | PTE_VALID | PTE_TABLE | PTE_AF;
    } else {
        uintptr_t p = l2_table[l2_idx] & 0x0000FFFFFFFFF000ULL;
        l3_table = (uint64_t *)phys_to_virt(p);
    }

    /* Seviye 3 Sayfa Girisi (PTE) */
    uint64_t pte = (paddr & 0x0000FFFFFFFFF000ULL) | PTE_VALID | PTE_PAGE | PTE_AF | PTE_SH_INNER;

    if (flags & VMM_FLAG_WRITE)
        pte |= (flags & VMM_FLAG_USER) ? PTE_AP_RW_ALL : PTE_AP_RW_EL1;
    else
        pte |= (flags & VMM_FLAG_USER) ? PTE_AP_RO_ALL : PTE_AP_RO_EL1;

    if (!(flags & VMM_FLAG_EXEC))
        pte |= PTE_PXN | PTE_UXN;

    if (flags & VMM_FLAG_NOCACHE)
        pte |= PTE_ATTRINDX(1); /* Device/Non-cacheable */
    else
        pte |= PTE_ATTRINDX(0); /* Normal cacheable */

    l3_table[l3_idx] = pte;

    /* TLB gecersizlestirme (ARM64 TLBI) */
    __asm__ volatile("tlbi vaae1is, %0" :: "r"(vaddr >> 12) : "memory");
    __asm__ volatile("dsb ish" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    return 0;
}

int vmm_unmap_page(uint64_t *l0_table, uintptr_t vaddr)
{
    if (!l0_table) return -1;

    size_t l0_idx = (vaddr >> 39) & 0x1FF;
    size_t l1_idx = (vaddr >> 30) & 0x1FF;
    size_t l2_idx = (vaddr >> 21) & 0x1FF;
    size_t l3_idx = (vaddr >> 12) & 0x1FF;

    if (!(l0_table[l0_idx] & PTE_VALID)) return -1;
    uint64_t *l1_table = (uint64_t *)phys_to_virt(l0_table[l0_idx] & 0x0000FFFFFFFFF000ULL);

    if (!(l1_table[l1_idx] & PTE_VALID)) return -1;
    uint64_t *l2_table = (uint64_t *)phys_to_virt(l1_table[l1_idx] & 0x0000FFFFFFFFF000ULL);

    if (!(l2_table[l2_idx] & PTE_VALID)) return -1;
    uint64_t *l3_table = (uint64_t *)phys_to_virt(l2_table[l2_idx] & 0x0000FFFFFFFFF000ULL);

    l3_table[l3_idx] = 0;

    __asm__ volatile("tlbi vaae1is, %0" :: "r"(vaddr >> 12) : "memory");
    __asm__ volatile("dsb ish" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    return 0;
}

uintptr_t vmm_virt_to_phys(uint64_t *l0_table, uintptr_t vaddr)
{
    if (!l0_table) return 0;

    size_t l0_idx = (vaddr >> 39) & 0x1FF;
    size_t l1_idx = (vaddr >> 30) & 0x1FF;
    size_t l2_idx = (vaddr >> 21) & 0x1FF;
    size_t l3_idx = (vaddr >> 12) & 0x1FF;

    if (!(l0_table[l0_idx] & PTE_VALID)) return 0;
    uint64_t *l1 = (uint64_t *)phys_to_virt(l0_table[l0_idx] & 0x0000FFFFFFFFF000ULL);

    if (!(l1[l1_idx] & PTE_VALID)) return 0;
    uint64_t *l2 = (uint64_t *)phys_to_virt(l1[l1_idx] & 0x0000FFFFFFFFF000ULL);

    if (!(l2[l2_idx] & PTE_VALID)) return 0;
    uint64_t *l3 = (uint64_t *)phys_to_virt(l2[l2_idx] & 0x0000FFFFFFFFF000ULL);

    if (!(l3[l3_idx] & PTE_VALID)) return 0;
    return (l3[l3_idx] & 0x0000FFFFFFFFF000ULL) | (vaddr & 0xFFFULL);
}
