#!/usr/bin/env bash
# ==============================================================================
# Quantro OS - Termux:X11 Canlı Görsel Ekran Başlatıcı
# Windows 11 Fluent Design & AArch64 Çekirdeğini Termux:X11'e Yansıtır
# ==============================================================================
set -e

PROJECT_DIR="/root/projects/Quantro-OS"
cd "$PROJECT_DIR"

echo "================================================================="
echo "   QUANTRO OS - TERMUX:X11 CANLI MASAÜSTÜ ARAYÜZÜ BAŞLATILIYOR  "
echo "================================================================="

# 1. X11 Soketini PRoot Konteynerine Bağla
mkdir -p /tmp/.X11-unix
if [ -d "/data/data/com.termux/files/usr/tmp/.X11-unix" ]; then
    ln -sf /data/data/com.termux/files/usr/tmp/.X11-unix/X0 /tmp/.X11-unix/X0 2>/dev/null || true
fi

# 2. Termux:X11 Sunucusu Kontrolü
if ! pgrep -f "termux-x11" >/dev/null 2>&1; then
    echo "[*] Termux:X11 sunucusu başlatılıyor..."
    if command -v termux-x11 >/dev/null 2>&1; then
        termux-x11 :0 -ac >/dev/null 2>&1 &
        sleep 1
    fi
fi
echo "[✓] Termux:X11 Sunucusu Aktif (DISPLAY=:0)"

# 3. Android Ekranında Termux:X11 Uygulamasını Ön Plana Getir
if [ -x "/data/data/com.termux/files/usr/bin/am" ]; then
    echo "[*] Termux:X11 Android ekranı ön plana getiriliyor..."
    /data/data/com.termux/files/usr/bin/am start --user 0 -n com.termux.x11/com.termux.x11.MainActivity >/dev/null 2>&1 || true
fi

# 4. ESP ve Limine Önyükleyicisini Doğrula
echo "[*] ESP ve Çekirdek İmajı Hazırlanıyor..."
make esp

# 5. QEMU ile Quantro OS'u Termux:X11 Penceresinde Tam Ekran Başlat
echo "[*] Quantro OS (Windows 11 Edition) Termux:X11 Ekranına Yansıtılıyor..."
echo "-----------------------------------------------------------------"
echo ">>> Termux:X11 uygulamasına geçin; işletim sistemi ekranınızda! <<<"
echo "-----------------------------------------------------------------"

export DISPLAY=:0
export SDL_VIDEODRIVER=x11
export GDK_BACKEND=x11

exec qemu-system-aarch64 \
    -M virt \
    -cpu cortex-a53 \
    -m 1G \
    -bios /usr/share/qemu-efi-aarch64/QEMU_EFI.fd \
    -drive file=fat:rw:esp,format=raw,if=virtio \
    -device ramfb \
    -device virtio-gpu-pci,xres=1280,yres=800 \
    -device virtio-tablet-device \
    -device virtio-keyboard-device \
    -display sdl \
    -serial file:serial.log
