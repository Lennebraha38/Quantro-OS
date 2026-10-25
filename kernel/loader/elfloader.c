#include "elfloader.h"
#include "uart.h"
#include "kernel.h"
#include "vfs.h"
#include "pmm.h"
#include "vmm.h"
#include "proc.h"

void elfloader_init(void)
{
    uart_puts("[loader:elf] AArch64 ELF64 Dynamic Binary Loader hazir\r\n");
}

int elfloader_validate(const void *elf_data, size_t size)
{
    if (!elf_data || size < sizeof(elf64_ehdr_t))
        return -1;

    const elf64_ehdr_t *ehdr = (const elf64_ehdr_t *)elf_data;

    /* Sihirli baytlar: 0x7F 'E' 'L' 'F' */
    if (ehdr->e_ident[0] != 0x7F ||
        ehdr->e_ident[1] != 'E'  ||
        ehdr->e_ident[2] != 'L'  ||
        ehdr->e_ident[3] != 'F') {
        uart_puts("[elf] HATA: Gecersiz ELF imzasi!\r\n");
        return -1;
    }

    /* 64-bit mi? */
    if (ehdr->e_ident[4] != 2) {
        uart_puts("[elf] HATA: 64-bit ELF degil!\r\n");
        return -2;
    }

    /* Little Endian mi? */
    if (ehdr->e_ident[5] != 1) {
        uart_puts("[elf] HATA: Little Endian degil!\r\n");
        return -3;
    }

    /* AArch64 (ARM64) mimarisi mi? (183) */
    if (ehdr->e_machine != EM_AARCH64) {
        uart_printf("[elf] HATA: Desteklenmeyen mimari (Machine ID: %u, Beklenen: 183)!\r\n",
                    (unsigned)ehdr->e_machine);
        return -4;
    }

    return 0;
}

int elfloader_load(const void *elf_data, size_t size, uint64_t *entry_out)
{
    if (elfloader_validate(elf_data, size) != 0)
        return -1;

    const elf64_ehdr_t *ehdr = (const elf64_ehdr_t *)elf_data;
    const uint8_t *base = (const uint8_t *)elf_data;

    if (ehdr->e_phoff + (ehdr->e_phnum * sizeof(elf64_phdr_t)) > size) {
        uart_puts("[elf] HATA: Program Header tablosu dosya sinirlari disinda!\r\n");
        return -5;
    }

    const elf64_phdr_t *phdrs = (const elf64_phdr_t *)(base + ehdr->e_phoff);
    int loaded_segments = 0;

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        const elf64_phdr_t *ph = &phdrs[i];

        if (ph->p_type == PT_LOAD) {
            uart_printf("[elf] PT_LOAD segmenti [%u]: vaddr=0x%llx, filesz=%llu, memsz=%llu, flags=0x%x\r\n",
                        i,
                        (unsigned long long)ph->p_vaddr,
                        (unsigned long long)ph->p_filesz,
                        (unsigned long long)ph->p_memsz,
                        (unsigned)ph->p_flags);

            /* Hedef bellek alanini tahsis et ve yukle */
            if (ph->p_memsz > 0) {
                uintptr_t page = pmm_alloc_pages((ph->p_memsz + 4095) / 4096);
                if (page) {
                    /* Veriyi kopyala */
                    if (ph->p_filesz > 0 && ph->p_offset + ph->p_filesz <= size) {
                        memcpy((void *)page, base + ph->p_offset, ph->p_filesz);
                    }
                    /* Kalan kismi (BSS) sifirla */
                    if (ph->p_memsz > ph->p_filesz) {
                        memset((void *)(page + ph->p_filesz), 0, ph->p_memsz - ph->p_filesz);
                    }
                }
            }
            loaded_segments++;
        }
    }

    if (entry_out)
        *entry_out = ehdr->e_entry;

    uart_printf("[elf] Basariyla yuklendi! Segment sayisi=%d, Giris Noktasi (Entry)=0x%llx\r\n",
                loaded_segments,
                (unsigned long long)ehdr->e_entry);

    return 0;
}

int elfloader_exec_file(const char *filename)
{
    if (!vfs_exists(filename)) {
        uart_printf("[elf] HATA: '%s' dosyasi bulunamadi!\r\n", filename);
        return -1;
    }

    const char *data = vfs_read(filename);
    uint32_t size = vfs_size(filename);

    uint64_t entry_point = 0;
    int res = elfloader_load(data, size, &entry_point);
    if (res == 0) {
        /* Yeni bir process olustur */
        int pid = proc_create(filename, (task_entry_t)entry_point);
        uart_printf("[elf] Surec olusturuldu: %s (PID: %d, Entry: 0x%llx)\r\n",
                    filename, pid, (unsigned long long)entry_point);
        return pid;
    }
    return res;
}
