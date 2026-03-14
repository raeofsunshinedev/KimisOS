#include "vfs.h"
#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#define MODULE_NAME "KVFS"
vfile_t root_dir = {"/", VFILE_DIRECTORY};

// !TODO: Modify code to become thread-safe

void vfs_init(){
    mlog(MODULE_NAME, "Initializing VFS\n", MLOG_PRINT);
    root_dir.access.data.ptr = kmalloc(1);
    root_dir.access.data.size_pgs = 1;
}

void add_file(vfile_t *vfile, vfile_t *current_dir){
    return;
}

vfile_t *fcreate(char *name, VFILE_TYPE type, ...){
    return 0;
}

int fdelete(vfile_t *file_entry){
    return 0;
}

// navya was here https://github.com/novabansal
int fwrite(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count){
    return 0;
}

int fread(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count){
    return 0;
}

vfile_t *lookup(char *name, vfile_t dir){
    return 0;
}

vfile_t *fopen(char *name){
    
    return 0;
}