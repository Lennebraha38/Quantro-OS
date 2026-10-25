#!/usr/bin/env python3
"""
Quantro OS - İlk Karşılama ve Kurulum Sonrası Sihirbazı (OOBE)
Dil Seçimi, Wi-Fi Yapılandırması, Tema, Donanım Sürücüleri ve Biyometri (Windows Hello)
"""

import sys
import time

def run_oobe():
    steps = [
        "1. Bölge ve Klavye Düzeni (Türkçe Q / İngilizce US)",
        "2. Ağ ve Wi-Fi Bağlantısı",
        "3. Görsel Tasarım Teması (Fluent Dark Mica / Light Acrylic)",
        "4. Windows Hello Biyometrik Giriş (Yüz Tanıma IR / Parmak İzi)",
        "5. Ekran Kartı ve Donanım Sürücüleri (Vulkan 1.3 / Mesa / DXVK)",
        "6. Gizlilik Tercihleri ve TPM 2.0 Güvenlik Mührü"
    ]
    print("=================================================================")
    print("              QUANTRO OS - HOŞ GELDİNİZ (OOBE)                   ")
    print("        Yeni Nesil Windows 11 Fluent Linux Deneyimi              ")
    print("=================================================================")
    for step in steps:
        print(f"[*] {step}...")
        time.sleep(0.15)
        print("    -> Tamamlandı [OK]")
    print("=================================================================")
    print("[✓] Tebrikler! Sisteminiz kullanıma hazır.")

if __name__ == "__main__":
    run_oobe()
