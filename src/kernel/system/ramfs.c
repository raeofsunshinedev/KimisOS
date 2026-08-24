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
    // ramfs_open,
    ramfs_close,
    ramfs_rfopen,
};

vfile_t root_dir = {"/", FS_FILE_IS_DIR | FS_FILE_SYSTEM, &ramfs_ops};

void ramfs_init(){
    mlog(MODULE_NAME, "Initializing VFS\n", MLOG_PRINT);
    root_dir.private = 0;
    root_dir.size = 0;
}

vfile_t *search_dir(char *subname, vfile_t *directory){
    if(!directory) return 0;
    vfile_t **children = (vfile_t **)directory->private;
    if(!children){
        return 0;
    }
    for(int i = 0; i < directory->size / sizeof(vfile_t *); i++){
        if(children[i] && !strcmp(children[i]->name, subname)){
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
    if(pathname[0] == '/') path++;
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
    
    // vfile_t **buffer = (vfile_t **)start->private;
    vfile_t *entry = start;
    char *current_path = pathname;
    for(int i = 0; i < pathname_entries; i++){
        entry = search_dir(path_tokens[i], entry);
        current_path += strlen(path_tokens[i]);
        current_path += (current_path[0] == '/');
        if(!entry){
            break;
        }
        if(entry->flags & FS_FILE_MOUNT){
            if(!get_last_child){
                entry = 0;
                break;
            }
            entry = entry->fileops->rfopen(current_path, entry);
            break;
        }
    }
    kfree(path);
    kfree(path_tokens);
    return entry;
}

char *get_filename(char *path){
    char *filename = path + strlen(path) - 1;//get last character in pathname
    while(*filename != path[0] && (*filename != '/' || filename == path)) filename--;
    filename += filename != path;//if the new filename is at the beginning of the path, don't adjust for '/' character
    return filename;
}

int ramfs_resize(vfile_t *file, uint32_t new_size_bytes){
    if(new_size_bytes == file->size) return new_size_bytes;
    uint32_t new_size_pages = (new_size_bytes + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES;
    uint32_t old_size_pages = (file->size + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES;
    do{
        if(old_size_pages == new_size_pages){
            break;
        }
        void *new_ptr = kmalloc(new_size_pages);
        if(!new_ptr){
            mlog(MODULE_NAME, "Failed to allocate memory for virtual file!\n", MLOG_PRINT);
            return 0;
        }
        if(file->private){
            memcpy((uint32_t *)file->private, (uint32_t*)new_ptr, file->size);
            kfree(file->private);
        }
        file->private = new_ptr;
    }while(0);
    file->size = new_size_bytes;
    return new_size_bytes;
}

vfile_t *ramfs_create(vfile_t *root, char *path, FS_FILE_FLAGS flags){
    if(!path){
        return 0;
    }
    if(path[0] == '/') path++;
    // vfile_t *parent = resolve_path(path, root, false);
    vfile_t *parent = root;
    if(!parent || !parent->flags & FS_FILE_IS_DIR){
        mlog(MODULE_NAME, "Could not locate parent directory for %s\n", MLOG_PRINT, path);
        path--;
        return 0;
    }
    char *new_name = path;
    vfile_t **dirents = (vfile_t **)parent->private;
    uint32_t insert_index = 0;
    for(uint32_t i = 0; i < parent->size/sizeof(vfile_t *); i++){
        if(dirents[i]){
            continue;
        }
        //Empty dirent, can insert
        vfile_t *new_file = kmalloc(1);
        *new_file = (vfile_t){0};
        // new_file->name = new_name;
        strcpy(new_name, new_file->name);
        new_file->flags = flags;
        dirents[i] = new_file;
        new_file->fileops = &ramfs_ops;
        path--;
        return new_file;
    }
    //no empty dirents, must resize
    if( !ramfs_resize(parent, parent->size + 4)) return 0;
    dirents = parent->private;
    vfile_t *new_file = kmalloc(1);
    if(!new_file) return 0;
    *new_file = (vfile_t){0};
    strcpy(new_name, new_file->name);
    new_file->flags = flags;
    dirents[parent->size/(sizeof(vfile_t*)) - 1] = new_file;
    new_file->fileops = &ramfs_ops;
    new_file->block_size_bytes = 1;
    path--;
    return new_file;
}

int ramfs_translate_dir(vfile_t *file, void *buffer, uint32_t offset, uint32_t count){
    uint32_t dirents_requested = count / sizeof(vfile_t);
    uint32_t dirents_availible = file->size / sizeof(vfile_t *);
    uint32_t to_copy = unsigned_min(dirents_availible, dirents_requested);
    vfile_t **dirents = (vfile_t **)file->private;
    for(uint32_t i = 0; i < to_copy; i++){
        memcpy(dirents[i], buffer + i * sizeof(vfile_t), sizeof(vfile_t));
    }
    return to_copy * sizeof(vfile_t);
}

int ramfs_delete(vfile_t *parent, char *child){
    return 0;
}
int ramfs_write(vfile_t *file, char *buffer, uint32_t offset, uint32_t count){
    //file->private can be null here, as ramfs_resize will handle assigning if it is not already assigned
    if(!file || !buffer || (file->flags & (FS_FILE_READ_ONLY | FS_FILE_IS_DIR | FS_FILE_MOUNT))) return 0;
    if(!ramfs_resize(file, count + offset)) return 0;
    memcpy(buffer, file->private + offset, count);
    return count;
}
int ramfs_read(vfile_t *file, char *buffer, uint32_t offset, uint32_t count){
    if(!file || !buffer || !file->private || file->flags & (FS_FILE_MOUNT)) return 0;
    if(file->flags & FS_FILE_IS_DIR){
        return ramfs_translate_dir(file, buffer, offset, count);
    }
    memcpy(file->private + offset, buffer, count);
    return count;
}
// vfile_t *ramfs_open(char *path){
//     return 0;
// }
void ramfs_close(vfile_t *file){
    file->refcount--;
    if(file->refcount < 0) file->refcount = 0;
    return;
}
vfile_t *ramfs_rfopen(char *name, vfile_t *parent){
    vfile_t *returnable = resolve_path(name, parent, true);
    // for(;;);
    return returnable;
}

vfile_t *get_root_dir(){
    return &root_dir;
}