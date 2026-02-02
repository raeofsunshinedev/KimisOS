#include <stdint.h>
#include "modlib.h"
#include "fs_driver.h"
#include "stdarg.h"
#define MODULE_NAME "KIFSM"

KOS_MAPI_FP api;

// void mount_fat32(vfile_t *dev_file, char *destination){
    
// }

int callback(vfile_t *device, MOUNT_OPERATION op, ...){
    if(op == MOUNT_NEW){
        //call mount functions to try and 
    }
}

//buffer is a pointer to a string that contains the filename, followed by a comma, followed by the mount destination. If there is no destination [i.e. no comma], command will be treated as unmount
int mount(vfile_t *file, char *buffer, uint32_t offset, uint32_t count){
    char filename_buffer[256];
    char destination_buffer[256];
    uint32_t comma_offset = 0;
    int i = 0;
    for(; i < 256; i++){
        if(buffer[i] == ',' || !buffer[i]){
            break;
        }
        filename_buffer[i] = buffer[i];
    }
    filename_buffer[i] = 0;
    comma_offset = i + 1;
    i = 0;
    for(; i < 256; i++){
        if(buffer[i] == 0){
            break;
        }
        destination_buffer[i] = buffer[i];
    }
    destination_buffer[i] = 0;
    vfile_t *src = fget_file(api, filename_buffer);
    if(!src){
        puts(api, MODULE_NAME, "src does not exist\n");
    }
    return 0;
}
int phony_read(vfile_t *file, char*buffer, uint32_t offset, uint32_t count){
    return 0;
}


void init(KOS_MAPI_FP module_api, uint32_t api_version){
    api = module_api;
    api(MODULE_API_PRINT, MODULE_NAME, "KIFSM Filesystem Driver Module v0.1.0\nSupported Filesystems:\n");
    
    return;
}