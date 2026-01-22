#pragma once
#include <stdint.h>

typedef enum vfile_type{
    VFILE_NULL,
    VFILE_POINTER,//virtual files
    VFILE_DEVICE,
    VFILE_MOUNT,//in the pointer passed to the function, must specify a read, write, open, create, and delete function.
    VFILE_DIRECTORY,
    VFILE_PDIR,//used for directories in physical filesystems.
    VFILE_FILE,//physical files
    VFILE_SYMLINK,
    VFILE_PIPE //yay we have pipes
}VFILE_TYPE; 

typedef enum mount_ops{
    MOUNT_NEW,
    MOUNT_UNMOUNT,
}MOUNT_OPERATION;


//this code is gonna be ***really*** unsafe
typedef struct virtual_file{
    char name[20];
    VFILE_TYPE type;
    uint32_t id;//to be assigned by driver;
    uint32_t mount_id;
    uint8_t lock;
    uint32_t size;
    struct virtual_file *parent;//should point to A: a virtual directory, or B: a mounted filesystem
    union{
        struct{
            int (*read)(struct virtual_file *file, void *data, uint32_t offset, uint32_t count);
            int (*write)(struct virtual_file *file, void *data, uint32_t offset, uint32_t count);
        }funcs;
        struct{
            void *ptr;
            uint32_t size_pgs;
        }__attribute__((packed))data;
    }access;
}vfile_t;

typedef enum fs_flags{
    FS_FILE_READ_ONLY = 1,
    FS_FILE_HIDDEN = 2,
    FS_FILE_SYSTEM = 4,
    FS_FILE_IS_DIR = 0x10,
    FS_FILE_ARCHIVE = 0x20
}FS_FILE_FLAGS;

typedef struct mount_funcs{
    int (*write)(vfile_t *file, void *data, uint32_t offset, uint32_t count);
    int (*read)(vfile_t *file, void *data, uint32_t offset, uint32_t count);
    int (*open)(char *filename, vfile_t *file);
    void (*delete)(vfile_t *file);
    void (*create)(char *filename, FS_FILE_FLAGS flags);
}mount_t;

typedef struct partition{
    uint8_t attributes;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t lba_start;
    uint32_t lba_size;
}__attribute__((packed))partition_t;

#define MBR_MAGIC 0xaa55

typedef struct mbr{
    char code[440];
    char id[4];
    char res[2];
    partition_t partitions[4];
    uint16_t magic;
}__attribute__((packed)) mbr_t;

void vfs_init();
vfile_t *fcreate(char *name, VFILE_TYPE type, ...);
void fdelete();
int fwrite(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count);
int fread(vfile_t *file_entry, void *byte_array, uint32_t offset, uint32_t count);
vfile_t *search_dir(char *name, vfile_t dir);
vfile_t *fopen(char *name);
// void vfs_del_mount_handler(uint32_t key);
// void vfs_add_mount_handler(int (*mount_handler)(vfile_t *device, MOUNT_OPERATION op, ...), uint32_t key);
void vfs_detect_partitions(vfile_t *file);