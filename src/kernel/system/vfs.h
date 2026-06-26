#pragma once
#include <stdint.h>

typedef enum fs_flags{
    FS_FILE_READ_ONLY = 1,
    FS_FILE_HIDDEN = 2,
    FS_FILE_SYSTEM = 4,
    FS_FILE_IS_DIR = 0x10,
    FS_FILE_ARCHIVE = 0x20,
    FS_FILE_PIPE = 0x40,
    FS_FILE_LINK = 0x80,
    FS_FILE_MOUNT = 0x100, // MUST be assigned to any file that represents a physical filesystem or physical device.
}FS_FILE_FLAGS;

typedef struct fileops{
    struct virtual_file *(*create)(char *path, FS_FILE_FLAGS flags);
    int (*delete)(struct virtual_file *file_entry);
    int (*write)(struct virtual_file *file_entry, void *data, uint32_t offset, uint32_t count);
    int (*read)(struct virtual_file *file_entry, void *data, uint32_t offset, uint32_t count);
    struct virtual_file *(*open)(char *path);
    void (*close)(struct virtual_file *file);
    // int (*readdir)(struct virtual_file* file, struct virtual_file *buffer, uint32_t count, uint32_t offset);
    struct virtual_file *(*rfopen)(char *name, struct virtual_file *parent);
} fileops_t;

typedef struct virtual_file{
    char name[212];
    uint16_t flags;
    
    fileops_t *fileops;
    uint32_t refcount; //filesystem MUST remain operational until all child refcounts == 0
    
    uint32_t id;//for use in drivers
    void *private; //also for use in drivers
    uint32_t size; //should be in bytes
    uint32_t offset; //for use in drivers
    
}vfile_t;

// vfile_t *rfopen(char *name, vfile_t dir);
vfile_t *fcreate(char *path, FS_FILE_FLAGS flags);
int fdelete(vfile_t* file_entry);
int fwrite(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count);
int fread(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count);
// int readdir(vfile_t* file, vfile_t *buffer, uint32_t offset, uint32_t count);

vfile_t *vfcreate(vfile_t *parent, char *relpath, FS_FILE_FLAGS flags);
vfile_t *rfopen(char *name, vfile_t *dir);
vfile_t *fclose(vfile_t *);
vfile_t *fopen(char *path);