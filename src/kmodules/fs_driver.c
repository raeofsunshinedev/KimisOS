#include <stdint.h>
#include "modlib.h"
#include "fs_driver.h"
#include "../kernel/shared/string.h"
#include "../kernel/shared/spinlock.h"
#include "stdarg.h"
#define MODULE_NAME "KIFSM"

KOS_MAPI_FP api;

void init(KOS_MAPI_FP module_api, uint32_t api_version);

const uint32_t FAT_CACHE_SIZE_ENTRIES = 16384;
const uint32_t MAX_MOUNT_COUNT = 64;
fat_mount_t *fat32_mounts;

fileops_t fat32_fileops = {0};

uint32_t first_free_open_file_index = 0;
uint32_t open_file_cache_size;
fat_open_file_t **open_file_cache;

module_t module_data = {
    init,
    0xfae00001,
    MODULE_NAME,
    0,
    0,
    0,
};

uint32_t fat32_cache_open(fat_open_file_t *file){
    for(uint32_t i = first_free_open_file_index; i < open_file_cache_size; i++){
        if(open_file_cache[i]){
            continue;
        }
        first_free_open_file_index = i;
        open_file_cache[i] = file;
        return i;
    }
    //failed to add to list, realloc and try again.
}

uint8_t fat32_check_valid(fat32_bpb_t *bpb){
    return bpb->signature == 0x28 || bpb->signature == 0x29;
}
uint8_t fat32_check_fsinfo(fsinfo_t *info){
    return (info->lead_sig == FAT32_FSINFO_LEAD_SIG && info->sig2 == FAT32_FSINFO_SIG2 && info->trail_sig == FAT32_FSINFO_TRAIL_SIG);
}

inline uint32_t translate_fat32_flags_to_vfile(uint32_t fat32_flags){
    return fat32_flags & 0x37;
}
inline uint32_t translate_vfile_flags_to_fat32(uint32_t vfile_flags){
    return vfile_flags & 0x3f;
}

void recache_fat32_table(uint32_t index, uint32_t size, uint32_t mount_index){
    uint8_t new_allocation  = 0;
    if(!fat32_mounts[mount_index].fat_cache){
        new_allocation = 1;
        fat32_mounts[mount_index].fat_cache = malloc(api, (FAT_CACHE_SIZE_ENTRIES * sizeof(uint32_t) + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES);
    }
    uint32_t min_fat_index = fat32_mounts[mount_index].fat_cache_start;
    uint32_t max_fat_index = min_fat_index + fat32_mounts[mount_index].fat_cache_size;
    
    uint32_t fat_start = fat32_mounts[mount_index].fat_start_sector * fat32_mounts[mount_index].mount_src->minimum_rw_size;
    api(MODULE_API_PRINT, MODULE_NAME, "FAT start byte: %x at file: %s\n", fat_start, fat32_mounts[mount_index].mount_src->name);
    
    if(!new_allocation){
        fwrite(api, fat32_mounts[mount_index].mount_src, fat32_mounts[mount_index].fat_cache, fat_start + min_fat_index * sizeof(uint32_t), FAT_CACHE_SIZE_ENTRIES * sizeof(uint32_t));
    }
    min_fat_index = index & 0xffffc000;
    fat32_mounts[mount_index].fat_cache_start = min_fat_index;
    fat32_mounts[mount_index].fat_cache_size = FAT_CACHE_SIZE_ENTRIES;
    // api(MODULE_API_PRINT, MODULE_NAME, "Old index: %x | New index: %x\n", fat32_mounts[mount_index].fat_cache_start, min_fat_index);
    api(MODULE_API_PRINT, MODULE_NAME, "Read offset: %x\n", fat_start + min_fat_index * sizeof(uint32_t));
    fread(api, fat32_mounts[mount_index].mount_src, fat32_mounts[mount_index].fat_cache, fat_start + min_fat_index * sizeof(uint32_t), FAT_CACHE_SIZE_ENTRIES * sizeof(uint32_t));
    
    puts(api, MODULE_NAME, "Wuh?\n");
    
    for(int i = 0; i < 8; i++){
        api(MODULE_API_PRINT, MODULE_NAME, "%x\n", fat32_mounts[mount_index].fat_cache[i]);
    }
}
//returns zero if OOB, returns 1 if in bounds
uint8_t fat32_check_bounds(uint32_t index, uint32_t mount_index){
    if(!fat32_mounts[mount_index].fat_cache_size){
        return 0;
    }
    uint32_t min_fat_index = fat32_mounts[mount_index].fat_cache_start;
    uint32_t max_fat_index = min_fat_index + fat32_mounts[mount_index].fat_cache_size;
    if(index < min_fat_index || index > max_fat_index){
        return 0;
    }
    
    return 1;
}

void fat32_set_next_cluster(uint32_t index, uint32_t value, uint32_t mount_index){
    uint32_t min_fat_index = fat32_mounts[mount_index].fat_cache_start;
    uint32_t max_fat_index = min_fat_index + fat32_mounts[mount_index].fat_cache_size;
    if(!fat32_check_bounds(index, mount_index)){
        recache_fat32_table(index, FAT_CACHE_SIZE_ENTRIES, mount_index);
    }
    fat32_mounts[mount_index].fat_cache[index - min_fat_index] = value;
}

uint32_t fat32_get_next_cluster(uint32_t index, uint32_t mount_index){
    uint32_t min_fat_index = fat32_mounts[mount_index].fat_cache_start;
    uint32_t max_fat_index = min_fat_index + fat32_mounts[mount_index].fat_cache_size;
    if(!fat32_check_bounds(index, mount_index)){
        recache_fat32_table(index, FAT_CACHE_SIZE_ENTRIES, mount_index);
    }
    return fat32_mounts[mount_index].fat_cache[index - min_fat_index];
}

uint32_t fat32_read_dirent(fat_dirent_t dirent, char *buffer, uint32_t mount_index){
    
}

fat_open_file_t *fat32_search_dir(char *path, fat_dirent_t *dir_data){
    return 0;
}

fat_open_file_t *resolve_path(char *path, vfile_t *parent){
    const uint32_t MAX_TOKENS = PAGE_SIZE_BYTES/4;
    //resolve path and get cluster
    if(!parent){
        return 0;
    }
    if(!path || path[0] == 0){
        api(MODULE_API_PRINT, MODULE_NAME, "Parent name: %s\n", parent->name);
        return parent;
    }
    uint32_t path_index = 0;
    // api(MODULE_API_PRINT, MODULE_NAME, "Path: %s\n", pathname);
    
    char *pathname = malloc(api, 1);
    if(!pathname){
        return 0;
    }
    strcpy(path, pathname);
    if(pathname[0] == '/') pathname++;
    char **path_tokens = malloc(api, 1);
    if(!path_tokens){
        free(api, pathname);
        return 0;
    }
    uint32_t pathname_entries = 0;
    char *pathtok = pathname;
    char *i = pathname;
    while (*i != '\0') {
        if (*i == '/') {
            *i = '\0';
            if (pathname_entries < MAX_TOKENS) {
                path_tokens[pathname_entries++] = pathtok;
            }
            pathtok = i + 1;
        }
        i++;
    }
    if (pathname_entries < MAX_TOKENS) {
        path_tokens[pathname_entries++] = pathtok;
    }
    fat_dirent_t *parent_dir = 0;
    for(uint32_t i = 0; i < pathname_entries; i++){
        api(MODULE_API_PRINT, MODULE_NAME, "Subpath: %s\n", path_tokens[i]);
        
        uint32_t size_to_alloc = parent_dir ? (parent_dir->size + PAGE_SIZE_BYTES - 1) / PAGE_SIZE_BYTES : 8;
        size_to_alloc += (!size_to_alloc * 8);
        
        api(MODULE_API_PRINT, MODULE_NAME, "Size to alloc: %d\n", size_to_alloc);
        fat_dirent_t *dir_data = malloc(api, size_to_alloc);
        recache_fat32_table(0, FAT_CACHE_SIZE_ENTRIES, parent->id);
        
        
        
        parent_dir = fat32_search_dir(path_tokens[i], dir_data);
    }
    //check if a reference is open
    //return reference
    free(api, pathname);
    free(api, path_tokens);
    return parent_dir;
}

vfile_t *fat32_open(char *path, vfile_t *parent){
    fat_open_file_t *file = resolve_path(path, parent);
    if(!file){
        return 0;
    }
    vfile_t *to_return = malloc(api, 1);
    to_return->fileops = &fat32_fileops;
    to_return->flags = translate_fat32_flags_to_vfile(file->file_flags);
    to_return->id = file->mount_index;
    //cache entry
    to_return->offset = fat32_cache_open(file);
    to_return->private = file;
    to_return->minimum_rw_size = 0;
    to_return->refcount = 1;
    return to_return;
}

vfile_t *fat32_create(vfile_t *parent, char *path, FS_FILE_FLAGS flags){
    
}

int fat32_delete(vfile_t *file){
    
}

int fat32_write(vfile_t *file, void *buffer, uint32_t offset, uint32_t count){
    
}

int fat32_read(vfile_t *file, void *buffer, uint32_t offset, uint32_t count){
    puts(api, MODULE_NAME, "Read called!\n");
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
    mount->fat_cache = 0;
    mount->fat_cache_size = 0;
    mount->fat_cache_start = 0;
    
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
    
    fat_open_file_t *root_dir = malloc(api, 1);
    
    root_dir->first_cluster = bpb->root_dir_cluster;
    root_dir->mount_index = index;
    
    mountfile->fileops = &fat32_fileops;
    mountfile->id = index;
    mountfile->size = filesize;
    mountfile->minimum_rw_size = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    mountfile->private = root_dir;
    
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
    
    fat32_fileops = (fileops_t){
        fat32_create,
        fat32_delete,
        fat32_write,
        fat32_read,
        fat32_close,
        fat32_open
    };
    
    return;
}