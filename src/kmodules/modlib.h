#pragma once
#include <stdint.h>

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
    MODULE_MESSAGE_HANDLER
};

typedef uint32_t (*KOS_MAPI_FP)(unsigned int function, ...);
typedef struct module{
    void *init_entry;
    uint32_t id;
    char name[16];
    uint8_t flags;
    uint16_t interrupts;
    uint32_t key;
    void (*message_handler)(uint32_t message, ...);
    void (*fini)(void);
}module_t;

typedef struct cpu_registers{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; 
    uint32_t int_no, pfa;
    uint32_t eip, cs, eflags, useresp, ss;
}__attribute__((packed))cpu_registers_t;

typedef enum mount_ops{
    MOUNT_NEW,
    MOUNT_UNMOUNT,
}MOUNT_OPERATION;

typedef enum vfile_type{
    VFILE_NULL,
    VFILE_POINTER,
    VFILE_DEVICE,
    VFILE_MOUNT,
    VFILE_DIRECTORY,
    VFILE_PDIR,//used for directories in physical filesystems.
    VFILE_FILE,
}VFILE_TYPE;

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
inline int fread(KOS_MAPI_FP api, vfile_t *file, char *buffer, uint32_t offset, uint32_t count){
    return api(MODULE_API_READ, file, buffer, offset, count);
}
inline int fwrite(KOS_MAPI_FP api, vfile_t *file, char *buffer, uint32_t offset, uint32_t count){
    return api(MODULE_API_WRITE, file, buffer, offset, count);
}
//read documenation for this one
inline vfile_t *fcreate(KOS_MAPI_FP api, char *filename, VFILE_TYPE type, char *pointer_write, char *size_read){
    return (void *)api(MODULE_API_CREAT, filename, type, pointer_write, size_read);
}
inline void puts(KOS_MAPI_FP api, char *mname, char *str){
    api(MODULE_API_PRINT, mname, str);
};
