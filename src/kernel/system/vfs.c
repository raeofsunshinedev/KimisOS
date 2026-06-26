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

vfile_t *fcreate(char *path, FS_FILE_FLAGS flags){
    if(!path){
        return;
    }
    
    char *name = kmalloc((strlen(path) + 4095)/4096);//don't make modifications to the original string
    strcpy(path, name);
    // printf("Sizeof: %d\n", sizeof(vfile_t));
    if(name[0] == '/') name++;//first slash just indicates that it's an absolute path, we don't want to include that into the filename.
    uint32_t name_length = strlen(name);
    // printf("%s, %d: ", name, strlen(name));
    uint32_t name_index = 0;
    uint32_t subpath_start = 0;
    
    vfile_t *new_file = 0;
    
    while(name[name_index]){//this condition shouldn't ever be triggered, but it's here to stop us if we're at the end.
        subpath_start = name_index;
        while(name[name_index] && name[name_index] != '/'){//split the string at directories
            name_index++;
        }
        name[name_index] = 0;
        // printf("%d, %s,", subpath_start, name + subpath_start);
        
        if(name_index >= name_length){//this should be the only exit condition
            printf("Creating file in dir; name: %s", name + subpath_start);
            break;
        }
        
        name_index++;
    }
    printf("\n");
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

// vfile_t *lookup(char *name, vfile_t dir){
//     return 0;
// }

int readdir(vfile_t* file, vfile_t* buffer, uint32_t offset, uint32_t count){
    return 0;
}

vfile_t *fopen(char *name){
    
    return 0;
}

//create file in vfs
vfile_t *vfs_create(char *path, FS_FILE_FLAGS flags){
    
}