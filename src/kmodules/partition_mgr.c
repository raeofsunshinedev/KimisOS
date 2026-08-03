#include "modlib.h"
#include "stdarg.h"
#include "../kernel/shared/string.h"
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

fileops_t mount_fileops = {};


int write_part(vfile_t *file_entry, void *buffer, uint32_t offset, uint32_t count){
    const int SECTOR_SIZE_BYTES;
    if(!file_entry || !buffer || !count){
        return 0;
    }
    uint32_t parent_index = file_entry->id;
    partent_t *partent = file_entry->private;
    vfile_t *parent = disk_references[parent_index].parent;
    // disk_references[parent_index].parent->fileops->read()
    parent->fileops->write(parent, buffer, offset + partent->start_lba * parent->minimum_rw_size, count);
    // parent->fileops->write(parent, buffer, offset + partent->start_lba * 512, count);
}

int read_part(vfile_t *file_entry, void *buffer, uint32_t offset, uint32_t count){
    const int SECTOR_SIZE_BYTES;
    if(!file_entry || !buffer || !count){
        return 0;
    }
    uint32_t parent_index = file_entry->id;
    partent_t *partent = file_entry->private;
    vfile_t *parent = disk_references[parent_index].parent;
    // disk_references[parent_index].parent->fileops->read()
    parent->fileops->read(parent, buffer, offset + partent->start_lba * parent->minimum_rw_size, count);
    // parent->fileops->read(parent, buffer, offset + partent->start_lba * parent, count);
}

int detect_partitions(vfile_t *file, uint32_t index){
    if(!file) return 0;
    
    const int PARTITIONS_PER_MBR = 4;
    const int PARTITIONS_PER_GPT = 128;
    const int MBR_BOOTABLE = 0x80;
    
    mbr_t *mbr = malloc(api, 1);
    uint32_t current_partition;
    fread(api, file, mbr, 0, 512);
    
    if(mbr->sig != 0xaa55){
        free(api, mbr);
        return -1;
    }
    uint8_t use_gpt = 0;
    uint8_t mbr_is_valid = 1;
    uint8_t found_valid_partition = 0;
    for(uint32_t i = 0; i < 4; i++){
        if(mbr->partition_table[i].type == 0xee){
            use_gpt = 1;
            break;
        }
        if(mbr->partition_table[i].flags != 0 && mbr->partition_table[i].flags != MBR_BOOTABLE){
            puts(api, MODULE_NAME, "Found invalid partition!\n");
            mbr_is_valid = 0;
            break;
        }
        if(mbr->partition_table[i].sector_count != 0 && mbr->partition_table[i].lba_address != 0){
            found_valid_partition = 1;
        }
        for(uint32_t j = 0; j < 4; j++){
            uint32_t a_end = mbr->partition_table[i].lba_address + mbr->partition_table[i].sector_count;
            uint32_t b_end = mbr->partition_table[j].lba_address + mbr->partition_table[j].sector_count;
            if(mbr->partition_table[i].lba_address < b_end && mbr->partition_table[j].lba_address < a_end && i != j){
                mbr_is_valid = 0;
                // puts(api, MODULE_NAME, "Found invalid partition\n");
                api(MODULE_API_PRINT, MODULE_NAME, "Invalid partition. Cause: %x, %x | %x, %x", mbr->partition_table[i].lba_address, b_end, mbr->partition_table[j].lba_address, a_end);
                break;
            }
        }
        if(!mbr_is_valid){
            break;
        }
    }
    if(!mbr_is_valid || !found_valid_partition){
        free(api, mbr);
        return -1;
    }
    if(use_gpt){
        puts(api, MODULE_NAME, "GPT Partitioning scheme not supported!\n");
        //Not currently supported. Sorry UEFI fans
        free(api, mbr);
        return -2;
    }
    uint32_t partition_count = 0;
    for(uint32_t i = 0; i < PARTITIONS_PER_MBR; i++){
        partition_table_entry_t partition = mbr->partition_table[i];
        if(partition.sector_count != 0){
            // disk_references[index].partitions[partition_count].file = fcreate(api, );
            uint32_t offset = partition.lba_address;
            uint32_t size = partition.sector_count;
            char *file_prefix = "/dev/disk/";
            char postfix[6] = "p";
            char number[4];//maximum of 3 characters in the number (127)
            
            itoa(partition_count, number, 10);
            
            char *fname = malloc(api, 1);
            strcpy(file_prefix, fname);
            strcat(file->name, fname);
            strcat(number, postfix);
            strcat(postfix, fname);
            
            vfile_t *new_file = fcreate(api, fname, FS_FILE_MOUNT);
            new_file->size = size;
            new_file->offset = partition_count;
            new_file->id = index;
            new_file->private = &(disk_references[index].partitions[partition_count]);
            new_file->fileops = &mount_fileops;
            
            disk_references[index].partitions[partition_count].file = new_file;
            disk_references[index].partitions[partition_count].sector_count = size;
            disk_references[index].partitions[partition_count].start_lba = offset;
            api(MODULE_API_PRINT, MODULE_NAME, "Creating file at index %d, partition %d, offset: %x, size: %x, fname: %s\n", index, partition_count, offset, size, fname);
            partition_count++;
        }
    }
    free(api, mbr);
    return 0;
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
    
    mount_fileops = (fileops_t){
        0,
        0,
        write_part,
        read_part,
        0,
        0,
    };
    
    return;
}