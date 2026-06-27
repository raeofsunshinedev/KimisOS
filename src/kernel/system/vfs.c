#include "vfs.h"
#include "ramfs.h"
#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#define MODULE_NAME "KVFS"

// !TODO: Modify code to become thread-safe
// 3 months later: does this count?


// void add_file(vfile_t *vfile, vfile_t *current_dir){
//     return;
// }

vfile_t *fcreate(char *path, FS_FILE_FLAGS flags){
    // return 0;
    vfile_t *returnable = vfcreate(get_root_dir(), path, flags);
    return 0;
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
        vfile_t *new_parent = rfopen(name, parent_dir);
        if(!new_parent){
            // printf("No new parent found!\n");
            kfree(name);
            return 0;
        }
        // printf("Recursed\n");
        new_file = vfcreate(new_parent, name+name_index+1, flags);
        fclose(new_parent);
    }
    kfree(name);
    return new_file;
}

int fdelete(vfile_t *file_entry){
    if(!file_entry || !file_entry->fileops || !file_entry->fileops->delete){
        return 0;
    }
    return file_entry->fileops->delete(file_entry);
}

int fwrite(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count){
    if(!file_entry || !file_entry->fileops || !file_entry->fileops->write ||!byte_array || count == 0){
        return 0;
    }
    return file_entry->fileops->write(file_entry, byte_array, offset, count);
}

int fread(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count){
    if(!file_entry || !file_entry->fileops || !file_entry->fileops->read || !byte_array || count == 0){
        return 0;
    }
    return file_entry->fileops->read(file_entry, byte_array, offset, count);
}

//fopen but relative to *dir
vfile_t *rfopen(char *name, vfile_t *dir){
    if(!dir || !name || !dir->fileops || !dir->fileops->rfopen){
        return 0;
    }
    return dir->fileops->rfopen(name, dir);
}

int readdir(vfile_t* file, vfile_t* buffer, uint32_t offset, uint32_t count){
    return 0;
}

vfile_t *fopen(char *name){
    if(!name){
        return 0;
    }
    vfile_t *root = get_root_dir();
    return root->fileops->open(name + (name[0] == '/'));
}

vfile_t *fclose(vfile_t *file){
    if(!file || !file->fileops || !file->fileops->close);
    file->fileops->close(file);
    return 0;
}