#include "vfs.h"
#include "uart.h"
#include "kernel.h"
#include "fat.h"

static vfs_file_t files[VFS_MAX_FILES];

void vfs_init(void)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        files[i].used = 0;
        files[i].size = 0;
        files[i].type = VFS_TYPE_FILE;
        files[i].perm = 0644;
        files[i].name[0] = '\0';
        files[i].data[0] = '\0';
    }

    /* Windows dosya sistemi uyumluluk hiyerarsisi */
    vfs_mkdir("C:");
    vfs_mkdir("C:/Windows");
    vfs_mkdir("C:/Windows/System32");
    vfs_mkdir("C:/Windows/System32/drivers");
    vfs_mkdir("C:/Users");
    vfs_mkdir("C:/Users/Quantro");
    vfs_mkdir("C:/Users/Quantro/Desktop");
    vfs_mkdir("C:/Program Files");

    /* Sistem dosyalari */
    vfs_create("readme.txt");
    vfs_write("readme.txt", "Quantro OS v0.4.0 (Windows 11 Architecture Parity)\nBuilt for AArch64 UEFI (Limine rev 2)");

    vfs_create("C:/Windows/System32/ntoskrnl.exe");
    vfs_write("C:/Windows/System32/ntoskrnl.exe", "MZ NT Kernel Image (Quantro OS AArch64 Subsystem)");

    vfs_create("C:/Windows/system.ini");
    vfs_write("C:/Windows/system.ini", "[boot]\nShell=QuantroExplorer.exe\nDrivers=virtio_gpu,virtio_net,pl011\nDisplay=1280x800x32");

    vfs_create("C:/Users/Quantro/Desktop/welcome.txt");
    vfs_write("C:/Users/Quantro/Desktop/welcome.txt", "Hos Geldiniz!\nQuantro OS masaustune basariyla girdiniz.\nWindows 11 benzeri arayuz ve komut satiri hazir.");

    uart_puts("[vfs] Quantro OS NTFS/VFS sanal dosya sistemi hazir\r\n");
}

int vfs_create(const char *name)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].type = VFS_TYPE_FILE;
            files[i].size = 0;
            files[i].perm = 0644;
            files[i].data[0] = '\0';
            strncpy(files[i].name, name, VFS_NAME_LEN - 1);
            files[i].name[VFS_NAME_LEN - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

int vfs_mkdir(const char *path)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, path) == 0)
            return 0;
    }
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].type = VFS_TYPE_DIR;
            files[i].size = 0;
            files[i].perm = 0755;
            files[i].data[0] = '\0';
            strncpy(files[i].name, path, VFS_NAME_LEN - 1);
            files[i].name[VFS_NAME_LEN - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

int vfs_write(const char *name, const char *data)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            strncpy(files[i].data, data, VFS_DATA_LEN - 1);
            files[i].data[VFS_DATA_LEN - 1] = '\0';
            files[i].size = strlen(files[i].data);
            return 0;
        }
    }
    /* Yoksa otomatik olustur */
    if (vfs_create(name) == 0)
        return vfs_write(name, data);
    return -1;
}

const char *vfs_read(const char *name)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0)
            return files[i].data;
    }
    return 0;
}

int vfs_delete(const char *name)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            files[i].used = 0;
            files[i].size = 0;
            files[i].name[0] = '\0';
            files[i].data[0] = '\0';
            return 0;
        }
    }
    return -1;
}

int vfs_rename(const char *oldn, const char *newn)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, oldn) == 0) {
            strncpy(files[i].name, newn, VFS_NAME_LEN - 1);
            files[i].name[VFS_NAME_LEN - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

int vfs_append(const char *name, const char *data)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            size_t cur = strlen(files[i].data);
            size_t rem = VFS_DATA_LEN - 1 - cur;
            if (rem > 0) {
                strncpy(files[i].data + cur, data, rem);
                files[i].data[VFS_DATA_LEN - 1] = '\0';
                files[i].size = strlen(files[i].data);
                return 0;
            }
            return -1;
        }
    }
    return -1;
}

int vfs_copy(const char *src, const char *dst)
{
    const char *d = vfs_read(src);
    if (!d)
        return -1;
    return vfs_write(dst, d);
}

int vfs_exists(const char *name)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0)
            return 1;
    }
    return 0;
}

uint32_t vfs_size(const char *name)
{
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0)
            return files[i].size;
    }
    return 0;
}

int vfs_file_count(void)
{
    int count = 0;
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used)
            count++;
    }
    return count;
}

const vfs_file_t *vfs_get_file(int index)
{
    if (index >= 0 && index < VFS_MAX_FILES && files[index].used)
        return &files[index];
    return 0;
}

void vfs_list(void)
{
    uart_puts("\r\n Dizin: C:\\\r\n\r\n");
    uart_puts(" Tip    Boyut        Ad\r\n");
    uart_puts(" --------------------------------------------\r\n");
    int count = 0;
    uint32_t total_size = 0;
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used) {
            count++;
            total_size += files[i].size;
            if (files[i].type == VFS_TYPE_DIR) {
                uart_printf(" <DIR>               %s\r\n", files[i].name);
            } else {
                uart_printf(" <FILE> %6u B      %s\r\n", files[i].size, files[i].name);
            }
        }
    }
    uart_printf("\r\n Toplam %d dosya/dizin, %u bayt kullanildi.\r\n", count, total_size);
}

void vfs_list_dir(const char *path)
{
    uart_printf("\r\n Dizin: %s\r\n\r\n", path);
    size_t plen = strlen(path);
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (files[i].used && strncmp(files[i].name, path, plen) == 0) {
            const char *sub = files[i].name + plen;
            if (*sub == '/' || *sub == '\\')
                sub++;
            if (*sub != '\0') {
                if (files[i].type == VFS_TYPE_DIR)
                    uart_printf(" <DIR>   %s\r\n", sub);
                else
                    uart_printf(" %6u B %s\r\n", files[i].size, sub);
            }
        }
    }
}

int vfs_mount(const char *point, const char *fstype)
{
    (void)point;
    if (strcmp(fstype, "fat") == 0 || strcmp(fstype, "fat32") == 0)
        return fat_mount();
    uart_puts("[vfs] bilinmeyen dosya sistemi turu\r\n");
    return -1;
}
