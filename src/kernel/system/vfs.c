#include "vfs.h"
#include "ramfs.h"
#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#define MODULE_NAME "KVFS"

// !TODO: Modify code to become thread-safe


void add_file(vfile_t *vfile, vfile_t *current_dir){
    return;
}

vfile_t *fcreate(char *path, FS_FILE_FLAGS flags){
    vfcreate(get_root_dir(), path, flags);
}

vfile_t *vfcreate(vfile_t *parent_dir, char *relpath, FS_FILE_FLAGS flags){
    if(!relpath){
        return 0;
    }
    
    char *name = kmalloc((strlen(relpath) + 4095)/4096);//don't make modifications to the original string
    strcpy(relpath, name);
    if(name[0] == '/') name++;//first slash just indicates that it's an absolute path, we don't want to include that into the filename.
    uint32_t name_length = strlen(name);
    uint32_t name_index = 0;
    uint32_t subpath_start = 0;
    
    vfile_t *new_file = 0;
    
    subpath_start = name_index;
    while(name[name_index] && name[name_index] != '/'){//split the string at directories
        name_index++;
    }
    name[name_index] = 0;
    // printf("%d, %s,", subpath_start, name + subpath_start);
    
    if(name_index >= name_length){//if there is no other directory
        // printf("Creating file; name: %s\n", name + subpath_start);
        new_file = parent_dir->fileops->create(name, flags);
    }
    else{
        vfile_t *new_parent = lookup(name, parent_dir);
        if(!new_parent){
            kfree(name);
            return 0;
        }
        new_file = vfcreate(new_parent, name+name_index+1, flags);
    }
    kfree(name);
    return new_file;
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

vfile_t *lookup(char *name, vfile_t *dir){
    return 0;
}

int readdir(vfile_t* file, vfile_t* buffer, uint32_t offset, uint32_t count){
    return 0;
}

vfile_t *fopen(char *name){
    
    return 0;
}