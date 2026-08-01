#include "modlib.h"
#include "stdarg.h"
#include "partition_mgr.h"
#include "../kernel/shared/spinlock.h"
#define MODULE_NAME "PARTMGR"

const uint32_t MAX_REFERENCES = 32;

KOS_MAPI_FP api = 0;

module_t module_data = {
    0,
    0xfae00002,
    MODULE_NAME,
    0,
    0,
    0,
};

spinlock_t partition_lock;
pref_t *disk_references;

int detect_partitions(vfile_t *file, uint32_t index){
    if(!file) return 0;
    
    mbr_t *mbr = malloc(api, 1);
    uint32_t current_partition;
    fread(api, file, mbr, 0, 512);
    
    if(mbr->sig == 0xaa55){
        puts(api, MODULE_NAME, "Valid MBR sig!\n");
    }
    
}

void device_add(vfile_t *file){
    spinlock_acquire(&partition_lock);
    
    for(uint32_t i = 0; i < MAX_REFERENCES; i++){
        if(disk_references[i].parent){
            continue;
        }
        disk_references[i].parent = file;
        int result = detect_partitions(file, i);
        if(result != 0){
            disk_references[i] = (pref_t){0};
        }
        break;
    }
    spinlock_release(&partition_lock);
}

void device_remove(vfile_t *file){
    spinlock_acquire(&partition_lock);
    for(int i = 0; i < MAX_REFERENCES; i++){
        if(disk_references[i].parent == file){
            int j = 0;
            while(j < 128 && disk_references[i].partitions[j].file){
                api(MODULE_API_DELET, disk_references[i].partitions[j].file);
                disk_references[i].partitions[j++] = (partent_t){0};
            }
            
            disk_references[i] = (pref_t){0};
            break;
        }
    }
    spinlock_release(&partition_lock);
    //search through open references, and if there are any partitions, delete the files and remove the reference.
    return;
}

int32_t message_handler(uint32_t message, ...){
    
    va_list args;
    va_start(args, message);
    
    if(message == MESSAGE_DEVICE_ADD || message == MESSAGE_DEVICE_REMOVE){
        vfile_t *file = va_arg(args, vfile_t *);
        // api(MODULE_API_PRINT, MODULE_NAME, "Called message handler! Message: %x\n", file);
        if(message == MESSAGE_DEVICE_ADD) device_add(file);
        else if (message == MESSAGE_DEVICE_REMOVE) device_remove(file);
        
    }
    
    va_end(args);
    return -1;
}

void fini(){
    free(api, disk_references);
    return;
}

void init(KOS_MAPI_FP module_api, uint32_t api_version){
    api = module_api;
    if(api_version != 0){
        api(MODULE_API_PRINT, MODULE_NAME, "Unsupported API version! Required: 0.0.0 | Reported: %d.%d.%d", api_version >> 16, (api_version >> 8) & 0xff, api_version & 0xff);
    }
    module_data.init_entry = init;
    module_data.fini = fini;
    api(MODULE_API_PRINT, MODULE_NAME, "PARTMGR Module v0.1.0\n");
    
    int32_t status = api(MODULE_API_REGISTER, &module_data);
    
    api(MODULE_MESSAGE_HANDLER, module_data.key, message_handler);
    spinlock_init(&partition_lock);
    disk_references = malloc(api, (sizeof(pref_t) * MAX_REFERENCES + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES);
    for(uint32_t i = 0; i < MAX_REFERENCES; i++){
        disk_references[i].parent = 0;
    }
    
    return;
}