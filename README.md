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
