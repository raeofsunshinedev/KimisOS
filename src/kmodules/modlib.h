#pragma once
#include <stdint.h>
#include "../kernel/shared/spinlock.h"

#define PAGE_SIZE_BYTES 4096

enum MODULE_API_FUNCS{
    
    MODULE_API_ADDFUNC,//adds function to kernel API handler
    MODULE_API_REGISTER, //registers the module as active using information provided from the module
    MODULE_API_DELFUNC, //delete function from kernel API handler
    MODULE_API_ADDINT, //set interrupt handler
    MODULE_API_DELINT, //delete interrupt handler
    MODULE_API_PRINT, //print to terminal
    MODULE_API_READ, //read from virtual file
    MODULE_API_WRITE, //write to virtual file
    MODULE_API_CREAT, //create a virtual file and assigns it to the the proper module (requires having a read and write function passed)
    MODULE_API_DELET, //delete a virtual file
    MODULE_API_OPEN,
    MODULE_API_MAP, //map physical address to virtual address
    MODULE_API_UNMAP, //unmap physical address to virtual address
    MODULE_API_PADDR, //get physical address of memory
    MODULE_API_MALLOC, //allocate memory in 4kb blocks
    MODULE_API_FREE, //free memory allocated by malloc
    MODULE_API_PMALLOC64K,
    MODULE_API_KMALLOC_PADDR,
    MODULE_MESSAGE_HANDLER,
    MODULE_API_DISPATCH_MESSAGE,
    MODULE_API_BLOCK_PID,
    MODULE_API_UNBLOCK_PID,
    MODULE_API_GET_CPID,
    MODULE_API_IS_INTERRUPT,
};

typedef uint32_t (*KOS_MAPI_FP)(unsigned int function, ...);
typedef struct module{
    void *init_entry;
    uint32_t id;
    char name[16];
    // uint8_t flags;
    uint32_t interrupts;
    uint32_t key;
    int32_t (*message_handler)(uint32_t message, ...);
    void (*fini)(void);
} module_t;

typedef struct cpu_registers{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; 
    uint32_t int_no, pfa;
    uint32_t eip, cs, eflags, useresp, ss;
}__attribute__((packed))cpu_registers_t;

enum MESSAGES{
    MESSAGE_MOUNT_FS,
    MESSAGE_UNMOUNT_FS,
    MESSAGE_PARTITION_DETECT,
    MESSAGE_PARTITION_REFRESH,
    
    MESSAGE_DEVICE_ADD,
    MESSAGE_DEVICE_REMOVE,
    
    MESSAGE_BROADCAST = 0x80000000, //placeholder
};

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
    struct virtual_file *(*create)(struct virtual_file *parent, char *path, FS_FILE_FLAGS flags);
    int (*delete)(struct virtual_file *parent, char *child);
    int (*write)(struct virtual_file *file_entry, void *data, uint64_t offset, uint64_t count);
    int (*read)(struct virtual_file *file_entry, void *data, uint64_t offset, uint64_t count);
    // struct virtual_file *(*open)(char *path);
    void (*close)(struct virtual_file *file);
    // int (*readdir)(struct virtual_file* file, struct virtual_file *buffer, uint32_t count, uint32_t offset);
    struct virtual_file *(*rfopen)(char *name, struct virtual_file *parent);
} fileops_t;

//note: this is EXACTLY 256 bytes. This is for simplicity's sake.
//PLEASE if you MUST reorganize or add fields, try and keep it to a power of 2?
typedef struct virtual_file{
    char name[76];
    uint16_t flags;
    fileops_t *fileops;
    uint32_t refcount; //filesystem MUST remain operational until all child refcounts == 0
    
    uint32_t id;//for use in drivers
    void *private; //also for use in drivers
    uint64_t size; //should be in bytes
    uint64_t offset; //for use in drivers
    
    uint16_t block_size_bytes;
    
    uint8_t owner_uid;
    uint8_t owner_gid;
    uint32_t last_modified;
    uint32_t created;
    uint16_t permissions; //same format as linux
    spinlock_t lock;
}vfile_t;

inline void *malloc(KOS_MAPI_FP api, uint32_t size_pages){
    return (void *)api(MODULE_API_MALLOC, size_pages);
}
inline void *free(KOS_MAPI_FP api, void *ptr){
    api(MODULE_API_FREE, ptr);
    return 0;
}
inline vfile_t *fopen(KOS_MAPI_FP api, char *filename){
    // vfile_t *file = malloc(api, 1);
    return (void *)api(MODULE_API_OPEN, filename);
}
inline int fread(KOS_MAPI_FP api, vfile_t *file, char *buffer, uint64_t offset, uint64_t count){
    return api(MODULE_API_READ, file, buffer, offset, count);
}
inline int fwrite(KOS_MAPI_FP api, vfile_t *file, char *buffer, uint64_t offset, uint64_t count){
    return api(MODULE_API_WRITE, file, buffer, offset, count);
}
//read documenation for this one
inline vfile_t *fcreate(KOS_MAPI_FP api, char *filename, FS_FILE_FLAGS type){
    return (void *)api(MODULE_API_CREAT, filename, type);
}
inline void puts(KOS_MAPI_FP api, char *mname, char *str){
    api(MODULE_API_PRINT, mname, str);
};
inline uint8_t is_interrupt(KOS_MAPI_FP api){
    return api(MODULE_API_IS_INTERRUPT);
}

inline uint32_t udiv64(uint64_t dividend, uint32_t divisor){
    uint32_t quotient;
    uint32_t remainder;
    uint32_t high = (uint32_t)(dividend >> 32);
    uint32_t low = (uint32_t)dividend;
    asm volatile ("divl %2\n" : "=d"(remainder), "=a"(quotient) : "r"(divisor), "0"(high), "1"(low));
    return quotient;
}