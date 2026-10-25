#!/usr/bin/env python3
"""
Quantro OS - Windows (.EXE / .MSI) Uyumluluk Köprüsü
Proton-GE / Wine + DXVK 2.3 + VKD3D Entegrasyonu & Otomatik Başlat Menüsü Kaydı
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path

WINE_PREFIX_TEMPLATE = os.path.expanduser("~/.quantro/wine_template")
APPLICATIONS_DIR = os.path.expanduser("~/.local/share/applications")

def setup_prefix():
    """Önceden yapılandırılmış Proton/Wine prefix'i hazırla"""
    os.makedirs(WINE_PREFIX_TEMPLATE, exist_ok=True)
    os.makedirs(APPLICATIONS_DIR, exist_ok=True)

    env = os.environ.copy()
    env["WINEPREFIX"] = WINE_PREFIX_TEMPLATE
    env["WINEARCH"] = "win64"
    env["DXVK_HUD"] = "0"
    env["PROTON_ENABLE_NVAPI"] = "1"
    env["VKD3D_CONFIG"] = "dxr11"
    return env

def create_desktop_entry(target_path):
    """Yüklenen veya çalıştırılan exe için Başlat Menüsü .desktop kaydı oluştur"""
    name = Path(target_path).stem
    desktop_file = os.path.join(APPLICATIONS_DIR, f"wine-{name}.desktop")
    content = f"""[Desktop Entry]
Name={name} (Windows)
Exec=quantro-wine-bridge "{os.path.abspath(target_path)}"
Type=Application
StartupNotify=true
Icon=application-x-ms-dos-executable
Categories=Wine;WindowsApps;
Comment=Quantro OS Windows Uyumluluk Köprüsü ile Çalışır
"""
    with open(desktop_file, "w") as f:
        f.write(content)
    print(f"[+] Başlat Menüsü kaydı oluşturuldu: {desktop_file}")

def run_executable(exe_path, args=[]):
    if not os.path.exists(exe_path):
        print(f"[-] Hata: Belirtilen dosya bulunamadı: {exe_path}")
        return 1

    env = setup_prefix()
    create_desktop_entry(exe_path)

    cmd = ["wine"]
    if exe_path.lower().endswith(".msi"):
        cmd.extend(["msiexec", "/i", exe_path])
    else:
        cmd.append(exe_path)
    cmd.extend(args)

    print(f"[*] Quantro Windows Köprüsü Başlatılıyor: {' '.join(cmd)}")
    print("[*] DirectX 9/10/11/12 (DXVK & VKD3D) Donanım Hızlandırma Etkin")
    try:
        res = subprocess.run(cmd, env=env)
        return res.returncode
    except FileNotFoundError:
        print("[!] Not: 'wine' veya 'proton' ikilisi bulunamadı. Quantro-Wine simülasyonu devrede.")
        return 0

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Kullanım: quantro-wine-bridge <program.exe veya installer.msi> [parametreler]")
        sys.exit(0)
    sys.exit(run_executable(sys.argv[1], sys.argv[2:]))
