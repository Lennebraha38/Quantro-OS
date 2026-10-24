# Benim-OS Pro - Build Sistemi (AArch64 / Limine)
#
# Cross-derleyici (aarch64-linux-gnu-*) kullaniliyorsa onu,
# degilse yerel derleyiciyi (clang/gcc) tercih eder.

ARCH := aarch64

ifeq ($(shell command -v aarch64-linux-gnu-gcc 2>/dev/null),)
  CROSS :=
  CC := clang
  LD := ld.lld
  OBJCOPY := llvm-objcopy
else
  CROSS := aarch64-linux-gnu-
  CC := $(CROSS)gcc
  LD := $(CROSS)ld
  OBJCOPY := $(CROSS)objcopy
endif

# QEMU ayarlari
QEMU := qemu-system-aarch64
QEMU_BIOS := /usr/share/qemu-efi-aarch64/QEMU_EFI.fd
QEMU_MACHINE := virt
QEMU_CPU := cortex-a53
QEMU_MEM := 1G

# Derleyici bayraklari (freestanding, libc bagsiz)
CFLAGS := -std=gnu17 -Wall -Wextra -O2 -pipe -ffreestanding -nostdinc \
          -fno-stack-protector -fno-pie -fno-pic \
          -mgeneral-regs-only \
          -Wno-unused-const-variable -Wno-type-limits \
          -ffunction-sections -fdata-sections \
          -Ikernel/include -Ikernel/arch/aarch64 \
          -D__AARCH64__ -DKERNEL_VERSION=\"0.2.0\"

# Linker bayraklari (yuksek yarimayfa, Limine uyumlu)
LDFLAGS := -T kernel/arch/aarch64/linker.ld \
           -nostdlib -z max-page-size=0x1000

KERNEL_DIR := kernel
BUILD_DIR := build
BOOTLOADER_DIR := bootloader
KERNEL_ELF := $(BUILD_DIR)/kernel

KERNEL_SRCS := $(shell find $(KERNEL_DIR) -name '*.c' -o -name '*.S')
KERNEL_OBJS := $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,\
               $(patsubst $(KERNEL_DIR)/%.S,$(BUILD_DIR)/%.o,$(KERNEL_SRCS)))

.PHONY: all clean run esp bootloader

all: $(KERNEL_ELF)

$(KERNEL_ELF): $(KERNEL_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $@
	@echo "Kernel olusturuldu: $@"

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# QEMU ile test (UEFI + Limine + virtio-gpu + seri konsol)
run: esp
	$(QEMU) -M $(QEMU_MACHINE) -cpu $(QEMU_CPU) -m $(QEMU_MEM) \
		-bios $(QEMU_BIOS) \
		-drive file=esp,format=raw,if=virtio \
		-device virtio-gpu-pci,xres=1280,yres=800 \
		-display none \
		-serial file:serial.log \
		-qmp unix:qmp.sock,server,nowait \
		-no-reboot &

# EFI sistem bolumu (FAT) olusturup Limine dosyalari kopyalar
esp: $(KERNEL_ELF) bootloader/BOOTAA64.EFI limine.conf
	rm -rf esp && mkdir -p esp/EFI/BOOT esp/boot
	cp bootloader/BOOTAA64.EFI esp/EFI/BOOT/BOOTAA64.EFI
	cp limine.conf esp/limine.conf
	cp $(KERNEL_ELF) esp/boot/kernel
	@echo "ESP hazir: esp/"

bootloader/BOOTAA64.EFI: bootloader
	@true

clean:
	rm -rf $(BUILD_DIR) esp serial.log qmp.sock
