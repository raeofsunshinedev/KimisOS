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

spinlock_t vfs_lock;

void vfs_init(){
    spinlock_init(&vfs_lock);
}

vfile_t *fcreate(char *path, FS_FILE_FLAGS flags){
    // return 0;
    spinlock_acquire(&vfs_lock);
    vfile_t *returnable = vfcreate(get_root_dir(), path, flags);
    spinlock_release(&vfs_lock);
    return returnable;
}

vfile_t *vfcreate(vfile_t *parent_dir, char *relpath, FS_FILE_FLAGS flags){
    if(!relpath){
        return 0;
    }
    
    char *name = kmalloc((strlen(relpath) + 4095)/4096);//don't make modifications to the original string
    strcpy(relpath, name);
    if(name[0] == '/') name++;//first slash just indicates that it's an absolute path, we don't want to include that into the filename.
    uint32_t name_length = strlen(name);
    if(name[name_length-1] == '/') name[name_length - 1] = 0;
    uint32_t name_index = 0;
    uint32_t subpath_start = 0;
    
    vfile_t *new_file = 0;
    
    subpath_start = name_index;
    while(name[name_index] && name[name_index] != '/'){//split the string at directories
        name_index++;
    }
    name[name_index] = 0;
    
    if(name_index >= name_length){//if there is no other directory
        new_file = parent_dir->fileops->create(parent_dir, name, flags);
    }
    else{
        vfile_t *new_parent = rfopen(name, parent_dir);
        if(!new_parent){
            printf("No new parent found from name: %s!\n", name);
            kfree(name);
            return 0;
        }
        new_file = vfcreate(new_parent, name+name_index+1, flags);
        // fclose(new_parent);
    }
    kfree(name);
    return new_file;
    // return 0;
}

int fdelete(vfile_t *file_entry){
    spinlock_acquire(&vfs_lock);
    if(!file_entry || !file_entry->fileops || !file_entry->fileops->delete){
        return 0;
    }
    int return_value = file_entry->fileops->delete(file_entry);
    spinlock_release(&vfs_lock);
    return return_value;
}
//Is expected to overwrite, not append if it is an actual file
//Devices and anything Not A File is exempt (i.e. Blockdevs, chardevs, etc)
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
    if(!strcmp(name, "/")){
        return root;
    }
    return root->fileops->rfopen(name + (name[0] == '/'), root);
}

vfile_t *fclose(vfile_t *file){
    if(!file || !file->fileops || !file->fileops->close) return 0;
    file->fileops->close(file);
    return 0;
}