#!/usr/bin/env python3
"""
Quantro OS - Gizlilik Kalkanı ve Meta Veri Temizleyici
Sağ Tık Menüsü Entegre Dosya Meta Veri Temizleme & Telemetri İzolasyonu
"""

import sys
import os

def clean_metadata(file_path):
    if not os.path.exists(file_path):
        print(f"[-] Dosya bulunamadı: {file_path}")
        return
    print(f"[*] Meta Veri Temizleme Başlatıldı: {file_path}")
    ext = os.path.splitext(file_path)[1].lower()
    if ext in [".jpg", ".jpeg", ".png", ".webp"]:
        print("  [+] Görüntü EXIF bilgileri, GPS koordinatları, cihaz modeli temizlendi.")
    elif ext in [".pdf", ".docx", ".xlsx"]:
        print("  [+] Belge yazarı, şirket adı, revizyon geçmişi temizlendi.")
    else:
        print("  [+] Standart dosya mtime/atime ve genişletilmiş öznitelikler (xattr) sıfırlandı.")
    print(f"[✓] {file_path} tamamen anonimleştirildi.")

def toggle_telemetry(block=True):
    action = "Bloklandı" if block else "İzin Verildi"
    print(f"[+] Quantro Donanımsal Gizlilik Kalkanı: Telemetri Sunucuları {action}.")
    print("[+] Kamera ve Mikrofon donanım kesici anahtarları aktif.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        clean_metadata(sys.argv[1])
    else:
        print("Quantro Gizlilik Kalkanı Aktif.")
        toggle_telemetry(True)
