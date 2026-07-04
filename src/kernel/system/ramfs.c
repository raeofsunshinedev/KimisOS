#include "vfs.h"
#include "ramfs.h"
#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#define MODULE_NAME "RAMFS"

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
    root_dir.size = PAGE_SIZE_BYTES;
    // printf("Sizeof VFILE_T: %d", sizeof(vfile_t));
}

vfile_t *search_dir(char *subname, vfile_t *directory){
    vfile_t **children = (vfile_t **)directory->private;
    for(int i = 0; i < directory->size / sizeof(vfile_t *); i++){
        if(!strcmp(children[i], subname)){
            return children[i];
        }
    }
    return 0;
}

//resolves the path relative to start
vfile_t *resolve_path(char *pathname, vfile_t *start, uint32_t get_last_child){
    const uint32_t MAX_TOKENS = PAGE_SIZE_BYTES/4;
    
    if(!start){
        return 0;
    }
    char *path = kmalloc(1);
    strcpy(pathname, path);
    
    char **path_tokens = kmalloc(1);
    uint32_t pathname_entries = 0;
    char *pathtok = path;
    char *i = path;
    while (*i != '\0') {
        if (*i == '/') {
            *i = '\0';
            if (pathname_entries < MAX_TOKENS) {
                path_tokens[pathname_entries++] = pathtok;
            }
            pathtok = i + 1;
        }
        i++;
    }
    if (pathname_entries < MAX_TOKENS) {
        path_tokens[pathname_entries++] = pathtok;
    }
    
    vfile_t **buffer = (vfile_t **)start->private;
    vfile_t *entry = start;
    for(int i = 0; i < pathname_entries - !get_last_child; i++){
        printf("%s", path_tokens[i]);
        entry = search_dir(path_tokens[i], entry);
        if(!entry){
            break;
        }
    }
    kfree(path);
    kfree(path_tokens);
    return entry;
}

char *get_filename(char *path){
    char *filename = path + strlen(path) - 1;//get last character in pathname
    while(*filename != '/' || filename == path) filename--;
    filename += filename != path;//if the new filename is at the beginning of the path, don't adjust for '/' character
    return filename;
}

vfile_t *ramfs_create(char *path, FS_FILE_FLAGS flags){
    if(!path){
        return 0;
    }
    vfile_t *parent = resolve_path(path, &root_dir, false);
    if(!parent){
        mlog(MODULE_NAME, "Could not locate parent directory for %s\n", MLOG_PRINT, path);
    }
    get_filename(path);
    
    return 0;
}

int ramfs_delete(vfile_t *file){
    return 0;
}
int ramfs_write(vfile_t *file, char *buffer, uint32_t offset, uint32_t count){
    return 0;
}
int ramfs_read(vfile_t *file, char *buffer, uint32_t offset, uint32_t count){
    return 0;
}
vfile_t *ramfs_open(char *path){
    return 0;
}
void ramfs_close(vfile_t *file){
    file->refcount--;
    if(file->refcount < 0) file->refcount = 0;
    return;
}
vfile_t *ramfs_rfopen(char *name, vfile_t *parent){
    return 0;
}

vfile_t *get_root_dir(){
    return &root_dir;
}