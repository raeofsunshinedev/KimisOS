#include "vfs.h"
#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#define MODULE_NAME "KVFS"
vfile_t root_dir = {"/", FS_FILE_IS_DIR | FS_FILE_SYSTEM};

// !TODO: Modify code to become thread-safe

void vfs_init(){
    mlog(MODULE_NAME, "Initializing VFS\n", MLOG_PRINT);
    root_dir.ptr = kmalloc(1);
    root_dir.size = 4096;//one page is 4096 bytes
}

void add_file(vfile_t *vfile, vfile_t *current_dir){
    return;
}

vfile_t *fcreate(char *name, FS_FILE_FLAGS flags){
    return 0;
}

int fdelete(vfile_t *file_entry){
    return 0;
}

int fwrite(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count){
    return 0;
}

int fread(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count){
    return 0;
}

// vfile_t *lookup(char *name, vfile_t dir){
//     return 0;
// }

int readdir(vfile_t* file, vfile_t *buffer, uint32_t offset, uint32_t count){
    return 0;
}

vfile_t *fopen(char *name){
    
    return 0;
}