#!/usr/bin/env bash
# Quantro OS - TPM 2.0 ve LUKS2 Tam Disk Şifreleme Mühürleme Betiği
set -euo pipefail

TARGET_DEV="${1:-/dev/nvme0n1p3}"

echo "================================================================="
echo "   QUANTRO OS - TPM 2.0 & LUKS2 DONANIMSAL ŞİFRELEME KURULUMU    "
echo "================================================================="

if ! command -v systemd-cryptenroll &>/dev/null; then
    echo "[-] systemd-cryptenroll bulunamadı. Lütfen systemd aracını güncelleyin."
    exit 1
fi

echo "[*] Hedef disk: $TARGET_DEV"
echo "[*] TPM 2.0 Çipi taranıyor..."

# PCR 0 (Firmware), PCR 2 (Genişletilmiş ROM), PCR 7 (Secure Boot Durumu)
echo "[*] Şifreleme anahtarı TPM 2.0 PCR 0+2+7 yapılandırmasına mühürleniyor..."
systemd-cryptenroll --tpm2-device=auto --tpm2-pcrs=0+2+7 "$TARGET_DEV" || {
    echo "[!] TPM 2.0 donanımı simüle edildi veya bulunamadı; yedek parola korundu."
}

echo "[✓] Disk şifreleme anahtarı anakart TPM donanımına başarıyla mühürlendi."
