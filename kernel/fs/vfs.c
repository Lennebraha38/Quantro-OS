#include "vfs.h"
#include "uart.h"
#include "string.h"
static vfs_file_t files[VFS_MAX_FILES];
void vfs_init(void){
    for(int i=0;i<VFS_MAX_FILES;i++) files[i].used=0;
    vfs_create("readme.txt");
    vfs_write("readme.txt","Quantro OS VFS v1");
    uart_puts("[vfs] in-memory filesystem ready\r\n");
}
int vfs_create(const char *name){
    for(int i=0;i<VFS_MAX_FILES;i++){
        if(!files[i].used){ files[i].used=1; files[i].size=0; files[i].data[0]='\0';
            int j=0; while(name[j] && j<VFS_NAME_LEN-1){ files[i].name[j]=name[j]; j++; } files[i].name[j]='\0';
            return 0; }
    }
    return -1;
}
int vfs_write(const char *name, const char *data){
    for(int i=0;i<VFS_MAX_FILES;i++){
        if(files[i].used && strcmp(files[i].name,name)==0){
            int j=0; while(data[j] && j<VFS_DATA_LEN-1){ files[i].data[j]=data[j]; j++; } files[i].data[j]='\0'; files[i].size=j; return 0;
        }
    }
    return -1;
}
const char *vfs_read(const char *name){
    for(int i=0;i<VFS_MAX_FILES;i++){
        if(files[i].used && strcmp(files[i].name,name)==0) return files[i].data;
    }
    return 0;
}
int vfs_delete(const char *name){
    for(int i=0;i<VFS_MAX_FILES;i++){
        if(files[i].used && strcmp(files[i].name,name)==0){ files[i].used=0; files[i].size=0; files[i].data[0]='\0'; files[i].name[0]='\0'; return 0; }
    }
    return -1;
}
int vfs_rename(const char *oldn,const char *newn){
    for(int i=0;i<VFS_MAX_FILES;i++){
        if(files[i].used && strcmp(files[i].name,oldn)==0){
            int j=0; while(newn[j] && j<VFS_NAME_LEN-1){ files[i].name[j]=newn[j]; j++; } files[i].name[j]='\0'; return 0;
        }
    }
    return -1;
}
int vfs_append(const char *name,const char *data){
    for(int i=0;i<VFS_MAX_FILES;i++){
        if(files[i].used && strcmp(files[i].name,name)==0){
            int j=files[i].size;
            while(data[j-files[i].size] && j<VFS_DATA_LEN-1){ files[i].data[j]=data[j-files[i].size]; j++; }
            files[i].data[j]='\0'; files[i].size=j; return 0;
        }
    }
    return -1;
}
int vfs_copy(const char *src,const char *dst){
    const char *d=vfs_read(src);
    if(!d) return -1;
    if(vfs_create(dst)!=0) return -1;
    return vfs_write(dst,d);
}
int vfs_exists(const char *name){
    for(int i=0;i<VFS_MAX_FILES;i++) if(files[i].used && strcmp(files[i].name,name)==0) return 1;
    return 0;
}
void vfs_list(void){
    uart_puts("Dosyalar:\r\n");
    for(int i=0;i<VFS_MAX_FILES;i++){
        if(files[i].used) uart_printf("  %s (%u bytes)\r\n", files[i].name, files[i].size);
    }
}
