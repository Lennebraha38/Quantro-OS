/*
 * Benim-OS Pro - Linker komut dosyasi (AArch64, Limine yuksek yarimayfa)
 *
 * Limine, ELF program basliklarindan yararlanarak cekirdegi yukler;
 * MMU zaten acik oldugundan vaddr ile paddr esit olmasi zorunlu degil,
 * ama basitlik icin identite yukleme kullanimdadir ve cekirdek
 * 0xffffffff80000000 baslangic adresinde derlenir.
 */
OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
ENTRY(_start)

PHDRS {
    requests PT_LOAD FLAGS(6);   /* R+W: Limine istekleri buraya yazilir */
    text PT_LOAD FLAGS(5);       /* R+X */
    rodata PT_LOAD FLAGS(4);     /* R */
    data PT_LOAD FLAGS(6);       /* R+W */
}

SECTIONS {
    . = 0xffffffff80000000;

    /* Limine istekleri ve delimiter'leri (base rev 2'de honore edilir) */
    .limine_requests_start : {
        KEEP(*(.limine_requests_start))
    } :requests

    .limine_requests : {
        KEEP(*(.limine_requests))
    } :requests

    .limine_requests_end : {
        KEEP(*(.limine_requests_end))
    } :requests

    . = ALIGN(4096);

    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(4096);

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(4096);

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    . = ALIGN(4096);
    _end = .;

    /DISCARD/ : {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.comment)
        *(.note*)
        *(.ARM.extab*)
        *(.ARM.exidx*)
    }
}
