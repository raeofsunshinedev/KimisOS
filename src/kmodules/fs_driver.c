#include <stdint.h>
#include "modlib.h"
#include "fs_driver.h"
#include "stdarg.h"
#define MODULE_NAME "KIFSM"

KOS_MAPI_FP api;

void init(KOS_MAPI_FP module_api, uint32_t api_version);

const uint32_t MAX_MOUNT_COUNT = 64;
fat_mount_t *fat32_mounts;

fileops_t fat32_fileops = {0};

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
uint8_t fat32_check_fsinfo(fsinfo_t *info){
    return (info->lead_sig == FAT32_FSINFO_LEAD_SIG && info->sig2 == FAT32_FSINFO_SIG2 && info->trail_sig == FAT32_FSINFO_TRAIL_SIG);
}

vfile_t *fat32_open(char *path, vfile_t *parent){
    
}

vfile_t *fat32_create(vfile_t *parent, char *path, FS_FILE_FLAGS flags){
    
}

int fat32_delete(vfile_t *file){
    
}

int fat32_write(vfile_t *file, void *buffer, uint32_t offset, uint32_t count){
    
}

int fat32_read(vfile_t *file, void *buffer, uint32_t offset, uint32_t count){
    
}

void fat32_close(vfile_t *file){
    
}

uint32_t fat32_mount(vfile_t *dev_file, char *destination, uint32_t offset){
    if(!dev_file){
        return -1;
    }
    
    char *bpb_buffer = malloc(api, 1);
    fread(api, dev_file, bpb_buffer, offset, 512);
    
    fat32_bpb_t *bpb = bpb_buffer;
    
    // api(MODULE_API_PRINT, MODULE_NAME, "Sizeof struct: %d, sig: %x, boot sig: %x\n", sizeof(fat32_bpb_t), bpb->signature, bpb->bootable_sig);
    if(!fat32_check_valid(bpb) && (bpb->sectors_small > 0)){
        api(MODULE_API_PRINT, MODULE_NAME, "Error: No valid BPB\n");
        free(api, bpb_buffer);
        return -1;
    }
    
    uint32_t index = 0;
    for(uint32_t i = 0; i < MAX_MOUNT_COUNT; i++){
        if(fat32_mounts[i].bpb){
            continue;
        }
        index = i;
        break;
    }
    
    fat_mount_t *mount = &(fat32_mounts[index]);
    
    *mount = (fat_mount_t){0};
    
    mount->bpb = bpb;
    mount->mount_src = dev_file;
    mount->max_clusters = bpb->sector_count/bpb->sectors_per_cluster;
    mount->data_start_sector = bpb->reserved_sectors + (bpb->fat_count * bpb->sectors_per_fat);
    mount->fat_start_sector = bpb->reserved_sectors;
    
    uint32_t filesize = (bpb->sector_count - (bpb->reserved_sectors + bpb->fat_count * bpb->sectors_per_fat)) * bpb->bytes_per_sector;
    
    uint32_t fsinfo_offset = bpb->fsinfo_sector * bpb->bytes_per_sector;
    
    fsinfo_t *fsinfo = malloc(api, 1);
    fread(api, dev_file, fsinfo, fsinfo_offset, sizeof(fsinfo_t));
    
    if(fat32_check_fsinfo(fsinfo)){
        mount->fat_search_start = fsinfo->first_free_cluster;
        mount->last_free_cluster_count = fsinfo->last_free_cluster_count;
    }
    // if(mount->last_free_cluster_count == 0xFFFFFFFF){
    //     //recompute
    // }
    
    vfile_t *mountfile = fcreate(api, destination, FS_FILE_MOUNT);
    
    if(!mountfile){
        free(api, bpb);
        free(api, fsinfo);
        *mount = (fat_mount_t){0};
        return -1;
    }
    
    mountfile->fileops = &fat32_fileops;
    mountfile->id = index;
    mountfile->size = filesize;
    mountfile->minimum_rw_size = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    
    spinlock_init(&(mount->spinlock));
    
    free(api, fsinfo);
    
    return 0;
}
    
int32_t message_handler(uint32_t message, ...){
    va_list args;
    va_start(args, message);
    
    if(message == MESSAGE_MOUNT_FS){
        vfile_t *device = va_arg(args, vfile_t *);
        char *dest = va_arg(args, char *);
        uint32_t offset = va_arg(args, uint32_t);
        return fat32_mount(device, dest, offset);
        // return 0;
    }
    return -1;
}

void init(KOS_MAPI_FP module_api, uint32_t api_version){
    api = module_api;
    if(api_version != 0){
        api(MODULE_API_PRINT, MODULE_NAME, "Unsupported API version! Required: 0.0.0 | Reported: %d.%d.%d", api_version >> 16, (api_version >> 8) & 0xff, api_version & 0xff);
    }
    api(MODULE_API_PRINT, MODULE_NAME, "KIFSM Filesystem Driver Module v0.1.0\nSupported Filesystems:\nFAT32\n");
    int32_t status = api(MODULE_API_REGISTER, &module_data);
    
    api(MODULE_MESSAGE_HANDLER, module_data.key, message_handler);
    // api(MODULE_API_PRINT, MODULE_NAME, "Sizeof: %x\n", MAX_MOUNT_COUNT);
    
    fat32_mounts = malloc(api,( MAX_MOUNT_COUNT * sizeof(fat_mount_t) + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES);
    for(uint32_t i = 0; i < sizeof(fat_mount_t) * MAX_MOUNT_COUNT; i++){
        ((uint8_t *)fat32_mounts)[i] = 0;
    }
    
    return;
}