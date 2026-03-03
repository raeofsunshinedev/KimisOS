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

// void mount_fat32(vfile_t *dev_file, char *destination){
    
// }

int32_t message_handler(uint32_t message, ...){
    puts(api, MODULE_NAME, "Called message handler!\n");
    return -1;
}

int phony_read(vfile_t *file, char*buffer, uint32_t offset, uint32_t count){
    return 0;
}


void init(KOS_MAPI_FP module_api, uint32_t api_version){
    api = module_api;
    api(MODULE_API_PRINT, MODULE_NAME, "KIFSM Filesystem Driver Module v0.1.0\nSupported Filesystems:\n");
    int32_t status = api(MODULE_API_REGISTER, &module_data);
    
    api(MODULE_MESSAGE_HANDLER, module_data.key, message_handler);
    api(MODULE_API_PRINT, MODULE_NAME, "Key: %x\n", module_data.key);
    
    return;
}