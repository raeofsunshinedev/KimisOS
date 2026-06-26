#include "vfs.h"
#include "ramfs.h"
#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#define MODULE_NAME "KVFS"


fileops_t ramfs_ops = 
{
    ramfs_create,
};

vfile_t root_dir = {"/", FS_FILE_IS_DIR | FS_FILE_SYSTEM, &ramfs_ops};

void ramfs_init(){
    mlog(MODULE_NAME, "Initializing VFS\n", MLOG_PRINT);
    root_dir.private = kmalloc(1);
    root_dir.size = 4096;//one page is 4096 bytes
}

vfile_t *ramfs_create(char *path, FS_FILE_FLAGS flags){
    return 0;
}

vfile_t *get_root_dir(){
    return &root_dir;
}