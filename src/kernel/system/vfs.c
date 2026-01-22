#include "vfs.h"
#include "../shared/kstdlib.h"
#include "../shared/memory.h"
#include "../shared/string.h"
#define MODULE_NAME "KVFS"
vfile_t root_dir = {"/", VFILE_DIRECTORY};

struct mount_handler{
    int (*callback)(vfile_t *device, MOUNT_OPERATION op, ...);
    uint32_t key;
}mount_handlers[32];

int vfs_link_exists(vfile_t *file){
    if(get_paddr(file) == 0 || file->type >= VFILE_MOUNT){
        return 0;
    }
    return 1;
}

void vfs_write_part(vfile_t *file, void *data, uint32_t offset, uint32_t count){
    if(!vfs_link_exists(file->parent) || count > file->size){
        return;
    }
    fread(file->parent, data, offset + file->id, count);
}
void vfs_read_part(vfile_t *file, void *data, uint32_t offset, uint32_t count){
    if(!vfs_link_exists(file->parent) || count > file->size){
        return;
    }
    fwrite(file->parent, data, offset + file->id, count);
}

void vfs_detect_partitions(vfile_t *file){
    char *buffer = kmalloc(1);
    fread(file, buffer, 0, 512);
    mbr_t *mbr = buffer;
    if(mbr->magic != MBR_MAGIC){
        mlog(MODULE_NAME, "No MBR!\n", MLOG_PRINT);
        mlog(MODULE_NAME, "Magic: %x\n", mbr->magic, MLOG_PRINT);
        return;
    }
    char cat[2] = {'a', 0};
    char nfname[512] = {0};
    strcpy(file->name, nfname);
    strcat(cat, nfname);
    char finalfname[512] = {0};
    strcpy("/dev/", finalfname);
    strcat(nfname, finalfname);
    for(int i = 0; i < 4; i++){
        partition_t part = mbr->partitions[i];
        if(part.type == 0xee){
            mlog(MODULE_NAME, "Found gpt\n", MLOG_PRINT);
            return;
        }
        if((part.attributes != 0 && part.attributes != 0x80) ){
            //check disk for filesystem
            mlog(MODULE_NAME, "No partitions in %s\n", MLOG_PRINT, file->name);
            // fcreate() new file representing partition
            vfile_t *nfile = fcreate(finalfname, VFILE_DEVICE, vfs_write_part, vfs_read_part);
            nfile->id = 0;
            nfile->size = file->size;
            nfile->parent = file;
            return;
        }
        
        finalfname[strlen(finalfname)-1]++;
    }
    mlog(MODULE_NAME, "Magic: %x\n", MLOG_PRINT, mbr->magic);
    return;
}

// void vfs_add_mount_handler(int (*mount_handler)(vfile_t *device, MOUNT_OPERATION op, ...), uint32_t key){
//     if(mount_handler == 0){
//         return;
//     }
//     for(uint32_t i = 0; i < 32; i++){
//         if(mount_handlers[i].callback == 0){
//             mount_handlers[i].callback = mount_handler;
//             mount_handlers[i].key = key;
//         }
//     }
// }
// void vfs_del_mount_handler(uint32_t key){
//     for(uint32_t i = 0; i < 32; i++){
//         if(mount_handlers[i].key == key){
//             mount_handlers[i].callback = 0;
//             mount_handlers[i].key = 0;
//         }
//     }
// }

void vfs_init(){
    mlog(MODULE_NAME, "Initializing VFS\n", MLOG_PRINT);
    root_dir.access.data.ptr = kmalloc(1);
    root_dir.access.data.size_pgs = 1;
}

void add_file(vfile_t *vfile, vfile_t *current_dir){
    vfile_t **dir_data = current_dir->access.data.ptr;
    int i = 0;
    while(dir_data[i] && (i * sizeof(vfile_t *))/4096 < current_dir->access.data.size_pgs){
        i++;
    }
    dir_data[i] = vfile;
    dir_data[i + 1] = 0;
}
/*
Long comment cause this one's a complicated one.
    Create a file of the specified path in name
    if the path leads to a mounted filesystem then
        use the mounted filesystem's specified create() function to create the file.
    else, create a pointer in the virtual path that points to a file entry for the new file.
    If this new file is a Directory or Pointer, specify a pointer to be used as a buffer to store data for this file, as well as the size of the buffer, in pages.
    If this new file is a Mounted filesystem, this pointer must be an array of functions as defined below:
        int fwrite(vfile_t *file, void *data, uint32_t offset, uint32_t count);
        //must write count bytes from data to file offset by offset bytes.
        int fread(vfile_t *file, void *data, uint32_t offset, uint32_t count);
        //must read count bytes from data to file offset by offset bytes.
        int fopen(char *filename, vfile_t *file);
        //must return a vfile_t struct written to the address specified in file, to represent a file named name (from the mount's filename)
        (eg. /mount/root/test is passed as /root/test)
        fcreate(char *filename, FS_FILE_FLAGS flags);
        FS_FILE_FLAGS is in the same format as FAT's file attribute byte of dirents.
        filenames are truncated, as with the previous example.
        fdelete(vfile_t *file);
        //the file specified in the file pointer must be deleted.
    else, if the file is a virtual representation of a device, of of a file (any miscellaneous file object that does not fit the other types) then
        specify a pointer to be used as the read function, as defined below:
                void read(struct virtual_file *file, void *data, uint32_t offset, uint32_t count);
        specify a pointer to be used as the write function, with the same function prototype as the function above
                void write(struct virtual_file *file, void *data, uint32_t offset, uint32_t count);
*/
vfile_t *fcreate(char *name, VFILE_TYPE type, ...){
    name += name[0]== '/';
    char *last = strtok(name, '/');
    char *tmp = last;
    uint32_t filename_offset = 0;
    vfile_t *current_dir = &root_dir; 
    vfile_t *tmpfile;
    
    va_list args;
    va_start(args, type);
    while(tmp != 0){
        // dir = tmp;
        tmpfile = search_dir(tmp, *current_dir);
        // printf("%s\n", name + filename_offset);
        if(!tmpfile){
            //create file in folder;
            vfile_t *vfile = kmalloc(1);
            vfile->type = type;
            vfile->parent = current_dir;
            uint32_t tmpnsize = strlen(tmp);
            if(tmp[tmpnsize - 1] == '/'){
                tmp[tmpnsize - 1] = 0;
            }
            strcpy(tmp, vfile->name);
            
            switch(type){
                case VFILE_POINTER:
                case VFILE_DIRECTORY:
                case VFILE_MOUNT:
                case VFILE_SYMLINK:
                    void *ptr = va_arg(args, void*);
                    vfile->access.data.ptr = ptr;
                    vfile->access.data.size_pgs = va_arg(args, uint32_t);
                    // *(uint32_t *)(ptr + (vfile->access.data.size_pgs * 4096)) = 0;
                    // what was the above line even for
                    add_file(vfile, current_dir);
                    break;
                case VFILE_FILE:
                case VFILE_DEVICE:
                    void *write = va_arg(args, void *);
                    void *read = va_arg(args, void *);
                    vfile->access.funcs.write = write;
                    vfile->access.funcs.read = read;
                    // strcpy
                    add_file(vfile, current_dir);
                    break;
            }
            return vfile; //allow drivers to make last minute changes before it's sent to the user
        }
        if(tmpfile->type == VFILE_SYMLINK){
            tmpfile = (vfile_t *)tmpfile->access.data.ptr;
        }
        switch(tmpfile->type){
            case VFILE_DIRECTORY:
                current_dir = tmpfile;
                break;
            case VFILE_MOUNT:
                ((mount_t*)(tmpfile->access.data.ptr))->create(name + filename_offset, type == VFILE_DIRECTORY ? FS_FILE_IS_DIR : 0);
                return (void *)-1;
            default:
            // mlog(MODULE_NAME, "Error: Cannot create file with same name as another file! %d\n", MLOG_ERR, tmpfile->type);
            return 0;
            break;
        }
        filename_offset += strlen(tmp) + 1;
        tmp = strtok(0, '/');
    }
    return 0;
}

//who needs a way to delete things?
//TODO: Write a function to delete files
void fdelete();

// navya was here https://github.com/novabansal
int fwrite(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count){
    //sorry navya, I deleted the comment and it's not coming back :(
    switch(file_entry->type){
        case VFILE_DIRECTORY:
        case VFILE_POINTER:
            memcpy(byte_array, file_entry->access.data.ptr + offset, count);
            return 0; //always successful. If it's not, there's been a page fault. Yk, quantum sort style.
            break;
        case VFILE_MOUNT:
            mount_t *funcs = file_entry->access.data.ptr;
            return funcs->write(file_entry, byte_array, offset, count);
        case VFILE_PDIR:
        case VFILE_FILE:
        case VFILE_DEVICE:
            return file_entry->access.funcs.write(file_entry, byte_array, offset, count);
            break;
        case VFILE_SYMLINK:
            return fwrite(file_entry->access.data.ptr, byte_array, offset, count);
    }
    return INT32_MIN;//how did we get here?
}

int fread(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count){
    switch(file_entry->type){
        case VFILE_DIRECTORY:
        case VFILE_POINTER:
            memcpy(file_entry->access.data.ptr + offset, byte_array, count);
            return 0; //always successful. If it's not, there's been a page fault. Yk, quantum sort style.
            break;
        case VFILE_MOUNT:
            mount_t *funcs = file_entry->access.data.ptr;
            return funcs->read(file_entry, byte_array, offset, count);
        case VFILE_PDIR:
        case VFILE_FILE:
        case VFILE_DEVICE:
            return file_entry->access.funcs.read(file_entry, byte_array, offset, count);
            break;
        case VFILE_SYMLINK:
            return fread(file_entry->access.data.ptr, byte_array, offset, count);
    }
    return INT32_MIN; //Okay, alright, funny joke guys but we really shouldn't be able to get here
}

vfile_t *search_dir(char *name, vfile_t dir){
    vfile_t **dir_data = (vfile_t **)dir.access.data.ptr;
    uint32_t i = 0;
    while(dir_data[i]){
        if(!strcmp(dir_data[i]->name, name)){
            return dir_data[i];
        }
        i++;
    }
    return 0;
}

vfile_t *fopen(char *name){
    if(name[0] == '/'){
        if(strlen(name) == 1){
            return &root_dir;
        }
        name++;
    }
    if(name[strlen(name) - 1] == '/'){
        name[strlen(name) - 1] = 0;
    }
    // printf("%s\n", name);
    char *dir = strtok(name, '/');
    // printf("%s", name);
    char *tmp = dir;
    uint32_t filename_offset = 0;
    vfile_t *current_dir = &root_dir; 
    vfile_t *tmpfile;
    while(tmp != 0){
        dir = tmp;
        tmpfile = search_dir(tmp, *current_dir);
        if(!tmpfile){
            // printf("not found\n");
            return 0;
        }
        if(tmpfile->type == VFILE_SYMLINK){
            tmpfile = (vfile_t *)tmpfile->access.data.ptr;
        }
        switch(tmpfile->type){
            case VFILE_DIRECTORY:
                // printf("found directory in %s: %s\n", current_dir->name, tmpfile->name);
                current_dir = tmpfile;
                break;
            case VFILE_MOUNT:
                vfile_t *file = kmalloc(1);
                ((mount_t*)(tmpfile->access.data.ptr))->open(name + filename_offset, file);
                return file;
            default:
                return tmpfile;
        }
        filename_offset += strlen(tmp) + 1;
        tmp = strtok(0, '/');
    }
    return tmpfile;
}