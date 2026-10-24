/*
 * Benim-OS Pro - AArch64 giris nusasi (Limine uzerinden yuklenir)
 *
 * Limine, MMU'yu acik birakir, TTBR1 yuksek yarimayfa sayfa
 * tablolarina isaret eder ve SP'yi (en az 64 KiB) ayarlar.
 * Biz kendi yiginimizi BSS'te acip kernel_main'i cagiririz.
 */

.section .text
.align 4
.global _start

_start:
    /* Tum kesimleri maskele (Limine zaten maskeli ama kesinlik icin) */
    msr daifset, #0xf

    /* VBAR'i temizle (GIC kurmayacagiz) */
    mov x0, xzr
    msr vbar_el1, x0

    /* Kendi yiginimizi olustur (BSS'teki stack) */
    adrp x0, stack_top
    add x0, x0, :lo12:stack_top
    mov sp, x0

    /* C koduna gec */
    bl kernel_main

    /* Geriye donmemesi gerekir; donerse sonsuz dongu */
    wfe
    b . - 4

/* ---- PSCI (Power State Coordination Interface) ------------------ */
/* QEMU virt makinesi PSCI'yi HVC yoluyla sunar. */

/* PSCI_VERSION: x0 = 0x84000000, dondurulen surum x0'da */
.align 2
.global psci_version
psci_version:
    mov x0, #0x84000000
    mov x1, xzr
    mov x2, xzr
    mov x3, xzr
    hvc #0
    ret

/* PSCI_SYSTEM_RESET: x0 = 0x84000009, donmemesi gerekir */
.align 2
.global psci_reset
psci_reset:
    mov x0, #0x84000009
    mov x1, xzr
    mov x2, xzr
    mov x3, xzr
    hvc #0
    ret

.section .bss.stack, "aw", %nobits
.align 4
stack_bottom:
    .skip 0x40000
stack_top:
