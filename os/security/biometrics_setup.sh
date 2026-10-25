#!/usr/bin/env bash
# Quantro OS - Windows Hello Karşılığı Biyometrik Giriş (Howdy IR + fprintd PAM)
set -euo pipefail

echo "================================================================="
echo "    QUANTRO OS - BİYOMETRİK GİRİŞ SİSTEMİ (WINDOWS HELLO)        "
echo "================================================================="

PAM_COMMON_AUTH="/etc/pam.d/common-auth"

echo "[*] PAM doğrulama kurallarına Biyometrik sağlayıcılar ekleniyor..."
# Howdy (IR Yüz Tanıma) ve fprintd (Parmak İzi) kuralı
if [ -f "$PAM_COMMON_AUTH" ]; then
    grep -q "pam_howdy.so" "$PAM_COMMON_AUTH" || {
        echo "auth sufficient pam_howdy.so" >> "$PAM_COMMON_AUTH"
        echo "[+] Howdy IR Yüz Tanıma PAM kuralı eklendi."
    }
    grep -q "pam_fprintd.so" "$PAM_COMMON_AUTH" || {
        echo "auth sufficient pam_fprintd.so" >> "$PAM_COMMON_AUTH"
        echo "[+] fprintd Parmak İzi PAM kuralı eklendi."
    }
fi

echo "[✓] Windows Hello biyometrik doğrulama mimarisi devrede."
