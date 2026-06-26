#include <stdint.h>
#include "modlib.h"
#include "fs_driver.h"
#include "stdarg.h"
#define MODULE_NAME "KIFSM"

KOS_MAPI_FP api;

void init(KOS_MAPI_FP module_api, uint32_t api_version);

module_t module_data = {
    init,
    0xfae00001,
    MODULE_NAME,
    0,
    0,
    0,
};

uint8_t fat32_check_valid(fat32_bpb_t *bpb){
    return bpb->signature == 0x28 || bpb->signature == 0x29;
}


uint32_t fat32_mount(vfile_t *dev_file, char *destination, uint32_t offset){
    char *bpb_buffer = malloc(api, 1);
    fread(api, dev_file, bpb_buffer, offset, 4096);
    fat32_bpb_t *bpb = bpb_buffer;
    
    api(MODULE_API_PRINT, MODULE_NAME, "Sizeof struct: %d, sig: %x, boot sig: %x\n", sizeof(fat32_bpb_t), bpb->signature, bpb->bootable_sig);
    if(!fat32_check_valid(bpb)){
        api(MODULE_API_PRINT, MODULE_NAME, "Error: No valid BPB\n");
        return 0;
    }
    puts(api, MODULE_NAME, "Valid BPB found!\n");
    
    // fat32_make_mount();
    
    return 1;
}

int32_t message_handler(uint32_t message, ...){
    puts(api, MODULE_NAME, "Called message handler!\n");
    
    va_list args;
    va_start(args, message);
    
    if(message == MESSAGE_MOUNT_FS){
        vfile_t *device = va_arg(args, vfile_t *);
        char *dest = va_arg(args, char *);
        uint32_t offset = va_arg(args, uint32_t);
        return fat32_mount(device, dest, offset);
    }
    return -1;
}

void init(KOS_MAPI_FP module_api, uint32_t api_version){
    api = module_api;
    api(MODULE_API_PRINT, MODULE_NAME, "KIFSM Filesystem Driver Module v0.1.0\nSupported Filesystems:\n");
    int32_t status = api(MODULE_API_REGISTER, &module_data);
    
    api(MODULE_MESSAGE_HANDLER, module_data.key, message_handler);
    api(MODULE_API_PRINT, MODULE_NAME, "Key: %x\n", module_data.key);
    
    return;
}