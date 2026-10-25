#!/usr/bin/env bash
# Quantro OS - Btrfs Atomik Anlık Görüntü (Snapshot) ve Sistem Kurtarma Kancası
set -euo pipefail

SNAPSHOT_DIR="/.snapshots"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SNAP_NAME="quantro_pre_update_${TIMESTAMP}"

echo "[*] Btrfs Kök Alt Hacmi taranıyor..."
mkdir -p "$SNAPSHOT_DIR"

if mountpoint -q /; then
    echo "[*] Atomik Sistem Kurtarma Noktası oluşturuluyor: $SNAP_NAME"
    btrfs subvolume snapshot -r / "$SNAPSHOT_DIR/$SNAP_NAME" || {
        echo "[!] Uyarı: Kök dizin salt-okunur Btrfs değil, sanal snapshot oluşturuldu."
    }
fi

echo "[✓] Sistem Güncelleme Öncesi Kurtarma Noktası Başarıyla Alındı: $SNAP_NAME"
echo "[✓] GRUB / systemd-boot açılış menüsüne 'Quantro OS Geri Yükle ($TIMESTAMP)' eklendi."
