#pragma once
#include "vfs.h"
#include <stdint.h>
#include "../shared/kstdlib.h"

#define MODULE_PRESENT 0x80
#define MODULE_INITRC 0x01
//semver is represented by hexadecimal
//first two digits are the major number
//second two digits are the minor number
//no patch number
#define MODULE_API_VERSION 0x0100

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
    MODULE_API_READDIR,
    MODULE_API_MAP, //map physical address to virtual address
    MODULE_API_UNMAP, //unmap physical address to virtual address
    MODULE_API_PADDR, //get physical address of memory
    MODULE_API_MALLOC, //allocate memory in 4kb blocks
    MODULE_API_FREE, //free memory allocated by malloc
    MODULE_API_PMALLOC64K,
    MODULE_API_KMALLOC_PADDR,
    MODULE_MESSAGE_HANDLER,
    MODULE_API_BLOCK_PID,
    MODULE_API_UNBLOCK_PID,
    MODULE_API_GET_CPID,
    MODULE_API_IS_INTERRUPT,
};

enum MESSAGES{
    MESSAGE_MOUNT_FS,
    MESSAGE_UNMOUNT_FS,
    MESSAGE_BROADCAST = 0x80000000, //placeholder
};


typedef struct module{
    void *init_entry;
    uint32_t id;
    char name[16];
    // uint8_t flags;
    uint32_t interrupts;
    uint32_t key;
    int32_t (*message_handler)(uint32_t message, ...);
    void (*fini)(void);
}module_t;

uint32_t dispatch_message(uint32_t message, ...);
void modules_init();
void module_start(void *ptr);