#include "vfs.h"
#include "ramfs.h"
#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#define MODULE_NAME "KVFS"


fileops_t ramfs_ops = 
{
    ramfs_create,
    ramfs_delete,
    ramfs_write,
    ramfs_read,
    ramfs_open,
    ramfs_close,
    ramfs_rfopen,
};

vfile_t root_dir = {"/", FS_FILE_IS_DIR | FS_FILE_SYSTEM, &ramfs_ops};

void ramfs_init(){
    mlog(MODULE_NAME, "Initializing VFS\n", MLOG_PRINT);
    root_dir.private = kmalloc(1);
    root_dir.size = PAGE_SIZE_BYTES;//one page is 4096 bytes
    // printf("Sizeof VFILE_T: %d", sizeof(vfile_t));
}

//resolves the path relative to start
vfile_t *resolve_path(char *pathname, vfile_t *start){
    // vfile_t **buffer = (vfile_t *)start->private;
    // for(int i = 0; i < start->size/4, i++){
        
    // }
}

vfile_t *ramfs_create(char *path, FS_FILE_FLAGS flags){
    
    return 0;
}

int ramfs_delete(vfile_t *file){
    
}
int ramfs_write(vfile_t *file, char *buffer, uint32_t offset, uint32_t count){
    
}
int ramfs_read(vfile_t *file, char *buffer, uint32_t offset, uint32_t count){
    
}
vfile_t *ramfs_open(char *path){
    
}
void ramfs_close(vfile_t *file){
    file->refcount--;
    if(file->refcount < 0) file->refcount = 0;
    return;
}
vfile_t *ramfs_rfopen(char *name, vfile_t *parent){
    
}

vfile_t *get_root_dir(){
    return &root_dir;
}