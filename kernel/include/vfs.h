#ifndef VFS_H
#define VFS_H
#include "types.h"
#define VFS_MAX_FILES 32
#define VFS_NAME_LEN 32
#define VFS_DATA_LEN 256
typedef struct { char name[VFS_NAME_LEN]; char data[VFS_DATA_LEN]; uint32_t size; int used; } vfs_file_t;
void vfs_init(void);
int vfs_create(const char *name);
int vfs_write(const char *name, const char *data);
const char *vfs_read(const char *name);
void vfs_list(void);
int vfs_delete(const char *name);
int vfs_rename(const char *oldn,const char *newn);
int vfs_append(const char *name,const char *data);
int vfs_copy(const char *src,const char *dst);
#endif
