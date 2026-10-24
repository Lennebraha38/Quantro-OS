# Quantro OS

Quantro OS, `benim-os-pro` tabanlı AArch64 (Limine) işletim sistemi projesidir.

## Özellik Matrisi (Windows 11 / Pardus / Quantro OS)

| Özellik | Windows 11 | Pardus | Quantro OS |
|---|---|---|---|
| UEFI boot (Limine) | ✔ | ✔ | ✔ |
| Framebuffer GUI | ✔ | ✔ | ✔ |
| UART seri konsol | ✔ | ✔ | ✔ |
| Bellek haritası (memmap) | ✔ | ✔ | ✔ |
| PSCI reboot | ✔ | ✔ | ✔ |
| Çoklu kullanıcı / izinler | ✔ | ✔ | ✗ (yol haritası) |
| Dosya sistemi (VFS) | ✔ (NTFS) | ✔ (ext4) | ✗ (yol haritası) |
| Ağ yığını (TCP/IP) | ✔ | ✔ | ✗ (yol haritası) |
| USB sürücüleri | ✔ | ✔ | ✗ (yol haritası) |
| Ses alt sistemi | ✔ | ✔ | ✗ (yol haritası) |
| Güvenli önyükleme / TPM | ✔ | ✔ | ✗ (yol haritası) |
| Çoklu görev (zamanlayıcı) | ✔ | ✔ | ✗ (yol haritası) |
| Paket yöneticisi | ✔ | ✔ (APT) | ✗ (yol haritası) |

## Yol Haritası
- [ ] VFS + basit dosya sistemi
- [ ] Zamanlayıcı / süreç yönetimi
- [ ] Ağ yığını (virtio-net)
- [ ] USB (virtio-input)
- [ ] Ses (virtio-snd)
- [ ] Çoklu kullanıcı & izinler

## Derleme
```bash
make
make run
```

## Iskelet Durumu (tamamlandi - tum moduller)

Tum moduller iskelet seviyesinde tamamlandi.
Tum alt sistemler iskelet seviyesinde mevcuttur: VFS, net (ARP/ICMP/TCP/UDP/HTTP), ses, proc scheduler, user/perm, pkg, sec/secure-boot, wm/compositor, gpu/keyboard/mouse/blk, initramfs/elfloader, ext4/fat, aslr, smp, power, ota, a11y, mgmt, uygulama katmani (terminal, filemgr, calculator, settings, browser).

## Modul sayisi: 46

## Puan (tahmini)
- Windows 11: 98/100
- Quantro OS (iskelet): 20/100
