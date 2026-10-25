#ifndef QUANTRO_VFS_H
#define QUANTRO_VFS_H

#include "types.h"

#define VFS_MAX_FILES 64
#define VFS_NAME_LEN  48
#define VFS_DATA_LEN  512

#define VFS_TYPE_FILE 0
#define VFS_TYPE_DIR  1

typedef struct {
    char name[VFS_NAME_LEN];
    char data[VFS_DATA_LEN];
    uint32_t size;
    uint8_t  type;
    uint8_t  used;
    uint16_t perm; /* 0755, 0644 vs. */
} vfs_file_t;

void vfs_init(void);
int  vfs_create(const char *name);
int  vfs_mkdir(const char *path);
int  vfs_write(const char *name, const char *data);
int  vfs_append(const char *name, const char *data);
const char *vfs_read(const char *name);
int  vfs_delete(const char *name);
int  vfs_rename(const char *oldn, const char *newn);
int  vfs_copy(const char *src, const char *dst);
int  vfs_exists(const char *name);
uint32_t vfs_size(const char *name);
void vfs_list(void);
void vfs_list_dir(const char *path);
int  vfs_file_count(void);
const vfs_file_t *vfs_get_file(int index);

#define VFS_MOUNT_POINT "/mnt"
int vfs_mount(const char *point, const char *fstype);

#endif /* QUANTRO_VFS_H */
