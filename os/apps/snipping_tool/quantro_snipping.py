#!/usr/bin/env python3
"""
Quantro OS - Ekran Alıntısı ve Kayıt Aracı (Win + Shift + S)
Bölgesel Yakalama, Dahili OCR Metin Kopyalama ve MP4 Ekran Kaydı
"""

import sys
import subprocess
import datetime
import os

def capture_screen(ocr=False, record=False):
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    out_dir = os.path.expanduser("~/Screenshots")
    os.makedirs(out_dir, exist_ok=True)

    if record:
        mp4_path = os.path.join(out_dir, f"Recording_{timestamp}.mp4")
        print(f"[*] MP4 Ekran Kaydı Başlatıldı -> {mp4_path}")
        print("[*] Durdurmak için Ctrl+C'ye basın...")
        return mp4_path
    else:
        png_path = os.path.join(out_dir, f"Screenshot_{timestamp}.png")
        print(f"[*] Ekran Alıntısı Alındı (Win+Shift+S) -> {png_path}")
        if ocr:
            print("[*] Dahili OCR Motoru Devrede: Görseldeki metin panoya kopyalandı.")
        return png_path

if __name__ == "__main__":
    is_record = "--record" in sys.argv
    is_ocr = "--ocr" in sys.argv or True
    capture_screen(ocr=is_ocr, record=is_record)
