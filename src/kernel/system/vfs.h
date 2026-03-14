#pragma once
#include <stdint.h>

typedef enum fs_flags{
    FS_FILE_READ_ONLY = 1,
    FS_FILE_HIDDEN = 2,
    FS_FILE_SYSTEM = 4,
    FS_FILE_IS_DIR = 0x10,
    FS_FILE_ARCHIVE = 0x20,
    FS_FILE_PIPE = 0x40,
    FS_FILE_LINK = 0x80
}FS_FILE_FLAGS;

typedef struct virtual_file{
    char name[256];
    FS_FILE_FLAGS flags;
    int (*delete)(struct virtual_file *file_entry);
    struct virtual_file *(*create)(char *name, FS_FILE_FLAGS flags);
    int (*write)(struct virtual_file *file_entry, void *data, uint32_t offset, uint32_t count);
    int (*read)(struct virtual_file *file_entry, void *data, uint32_t offset, uint32_t count);
    struct virtual_file *(*open)(char *name);
    struct virtual_file **(*lookup)(char *name);
    
    uint32_t id;//for use in drivers
    void * ptr; //also for use in drivers
    uint32_t size;
    uint32_t offset; //for use in drivers
    
}vfile_t;

void vfs_init();
vfile_t *fcreate(char *name, FS_FILE_FLAGS flags);
int fdelete(vfile_t* file_entry);
int fwrite(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count);
int fread(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count);
vfile_t *lookup(char *name, vfile_t dir);
vfile_t *fopen(char *name);