#include <stdint.h>
#include "modlib.h"
#include "fs_driver.h"
#include "../kernel/shared/string.h"
#include "../kernel/shared/spinlock.h"
#include "stdarg.h"
#define MODULE_NAME "KIFSM"

#define FAT32_EOC_MIN 0x0FFFFFF8
#define FAT32_EOC 0x0FFFFFFF

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
    uint32_t used_pages = (open_file_cache_size + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES;
    uint32_t new_pages = used_pages + 4;
    // api(MODULE_API_PRINT, MODULE_NAME, "Failed to add to list: Need pages: %d | Used pages: %d\n", new_pages, used_pages);
    fat_open_file_t **new_cache = malloc(api, new_pages);
    for(uint32_t i = 0; i < new_pages * PAGE_SIZE_BYTES / sizeof(uint32_t); i++){
        new_cache = 0;
    }
    memcpy(open_file_cache, new_cache, open_file_cache_size * sizeof(fat_open_file_t **));
    open_file_cache_size = (new_pages * PAGE_SIZE_BYTES)/sizeof(fat_open_file_t **);
    
    // api(MODULE_API_PRINT, MODULE_NAME, "New cache size: %x\n", open_file_cache_size);
    free(api, open_file_cache);
    open_file_cache = new_cache;
    
    return fat32_cache_open(file);
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
    
    uint32_t fat_start = fat32_mounts[mount_index].fat_start_sector * fat32_mounts[mount_index].mount_src->block_size_bytes;
    
    if(!new_allocation && fat32_mounts[mount_index].flags.cache_dirty){
        fwrite(api, fat32_mounts[mount_index].mount_src, fat32_mounts[mount_index].fat_cache, fat_start + min_fat_index * sizeof(uint32_t), FAT_CACHE_SIZE_ENTRIES * sizeof(uint32_t));
    }
    //clear the lower bits
    //FAT_CACHE_SIZE_ENTRIES should be a power of two
    min_fat_index = index & -FAT_CACHE_SIZE_ENTRIES;
    
    fat32_mounts[mount_index].fat_cache_start = min_fat_index;
    fat32_mounts[mount_index].fat_cache_size = FAT_CACHE_SIZE_ENTRIES;
    fat32_mounts[mount_index].flags.cache_dirty = 0;
    // puts(api, MODULE_NAME, "Pre read!\n");
    // api(MODULE_API_PRINT, MODULE_NAME, "Mount src id: %d\n", fat32_mounts[mount_index].mount_src->id);
    fread(api, fat32_mounts[mount_index].mount_src, fat32_mounts[mount_index].fat_cache, fat_start + min_fat_index * sizeof(uint32_t), FAT_CACHE_SIZE_ENTRIES * sizeof(uint32_t));
    // puts(api, MODULE_NAME, "Post read!\n");
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
        min_fat_index = fat32_mounts[mount_index].fat_cache_start;
    }
    fat32_mounts[mount_index].flags.cache_dirty = 1;
    fat32_mounts[mount_index].fat_cache[index - min_fat_index] = value;
    fat32_mounts[mount_index].fat_search_start = index;
}

uint32_t fat32_get_next_cluster(uint32_t index, uint32_t mount_index){
    uint32_t min_fat_index = fat32_mounts[mount_index].fat_cache_start;
    uint32_t max_fat_index = min_fat_index + fat32_mounts[mount_index].fat_cache_size;
    // api(MODULE_API_PRINT, MODULE_NAME, "Parent id: %d\n", fat32_mounts[mount_index].mount_src->id);
    if(!fat32_check_bounds(index, mount_index)){
        recache_fat32_table(index, FAT_CACHE_SIZE_ENTRIES, mount_index);
        min_fat_index = fat32_mounts[mount_index].fat_cache_start;
    }
    return fat32_mounts[mount_index].fat_cache[index - min_fat_index];
}

uint32_t fat32_get_free_cluster(uint32_t mount_index){
    fat_mount_t *mount = &fat32_mounts[mount_index];
    uint32_t search_start = mount->fat_search_start;
    uint32_t sector_count = mount->bpb->sector_count - mount->bpb->reserved_sectors - (mount->bpb->fat_count * mount->bpb->sectors_per_fat);
    uint32_t entry_count = sector_count / mount->bpb->sectors_per_cluster;
    // api(MODULE_API_PRINT, MODULE_NAME, "Sector count: %x | Entry count: %x | Reserved sectors: %x| Total sectors: %x", sector_count, entry_count, mount->bpb->reserved_sectors, mount->bpb->sector_count);
    for(uint32_t i = search_start; i < entry_count; i++){
        if(!fat32_get_next_cluster(i, mount_index)){
            mount->fat_search_start = i;
            // api(MODULE_API_PRINT, MODULE_NAME, "Found a free cluster at %d: %x", i, fat32_get_next_cluster(i, mount_index));
            return i;
        }
    }
    return 0;
}

uint32_t fat32_read_dirent(fat_dirent_t *dirent, char *buffer, uint32_t mount_index){
    uint32_t cluster = fat32_mounts[mount_index].bpb->root_dir_cluster;
    if(dirent && dirent->name[0] != 0){
        cluster = dirent->cluster_high << 16 | dirent->cluster_low;
    }
    fat_mount_t *mount = &(fat32_mounts[mount_index]);
    // api(MODULE_API_PRINT, MODULE_NAME, "Data Offset: %x\n", mount->data_start_sector * mount->bpb->bytes_per_sector);
    uint32_t cluster_number = 0;
    while(cluster < FAT32_EOC_MIN){
        // api(MODULE_API_PRINT, MODULE_NAME, "Cluster: %x\n", cluster);
        
        uint32_t cluster_offset_start = (mount->data_start_sector + ((cluster - 2) * mount->bpb->sectors_per_cluster)) * mount->bpb->bytes_per_sector;
        
        // api(MODULE_API_PRINT, MODULE_NAME, "Data Offset: %x\n", cluster_offset_start);
        // api(MODULE_API_PRINT, MODULE_NAME, "Mount_src->id: %d\n", mount->mount_src->id);
        
        fread(api, mount->mount_src, buffer + cluster_number * mount->bpb->sectors_per_cluster * mount->bpb->bytes_per_sector, cluster_offset_start, mount->bpb->bytes_per_sector * mount->bpb->sectors_per_cluster);
        // api(MODULE_API_PRINT, MODULE_NAME, "Mount_src->id: %s\n", mount->mount_src->id);
        
        cluster_number++;
        
        // api(MODULE_API_PRINT, MODULE_NAME, "Pre new cluster\n");
        cluster = fat32_get_next_cluster(cluster, mount_index);
    }
    return 0;
}

uint32_t fat32_build_filename_long(uint32_t start_index, char *filename, fat_dirent_t *dir_data){
    const uint32_t LFN_ENTRY_CHAR_COUNT = 13;
    if(!dir_data || !filename || dir_data[start_index].flags != FAT32_LONG_FILE_NAME){
        return start_index;
    }
    uint32_t index = start_index;
    while((dir_data[index].flags & FAT32_LONG_FILE_NAME) && dir_data[index].name[0]){
        fat_lfn_t *lfn_ent = (fat_lfn_t *)&(dir_data[index]);
        uint32_t filename_index_start = ((lfn_ent->entry_no & 0x3f) - 1) * LFN_ENTRY_CHAR_COUNT; //Strip `last entry` flag and zero index
        
        uint32_t i = 0;
        uint32_t j = 0;
        for(j = 0; j < sizeof(lfn_ent->name0)/sizeof(uint16_t); i++, j++){
            if(lfn_ent->name0[j] == 0xffff || lfn_ent->name0[j] == 0x0000) break;
            filename[i + filename_index_start] = lfn_ent->name0[j];
        }
        for(j = 0; j < sizeof(lfn_ent->name1)/sizeof(uint16_t); i++, j++){
            if(lfn_ent->name1[j] == 0xffff || lfn_ent->name1[j] == 0x0000) break;
            filename[i + filename_index_start] = lfn_ent->name1[j];
        }
        for(j = 0; j < sizeof(lfn_ent->name2)/sizeof(uint16_t); i++, j++){
            if(lfn_ent->name2[j] == 0xffff || lfn_ent->name2[j] == 0x0000) break;
            filename[i + filename_index_start] = lfn_ent->name2[j];
        }
        index++;
    }
    return index;
}

//could be cleaned up, but it works
//don't ask about the magic numbers, or why some of the numbers are the way they are, they just are
void fat32_copy_short_filename(uint32_t index, char *filename, fat_dirent_t *dir_data){
    uint32_t j = 0;
    for(; j < 8; j++){
        filename[j] = dir_data[index].name[j];
    }
    while((filename[j] == ' ' || filename[j] == 0) && j > 0){
        filename[j] = 0;
        j--;
    }
    filename[++j] = dir_data[index].name[8] ? '.' : 0;
    for(int i = 8; i < 11; i++){
        filename[j + i - 7] = dir_data[index].name[i];
    }
}

fat_dirent_t *fat32_search_dir(char *path, fat_dirent_t *dir_data, uint32_t *dirent_index){
    uint32_t i = 0;
    const int DIR_ENT_MAX = 65536;
    while(dir_data[i].name[0] && i < DIR_ENT_MAX){
        char filename[256] = {0};
        if(dir_data[i].flags == FAT32_LONG_FILE_NAME){
            i = fat32_build_filename_long(i, filename, dir_data);
        }
        else{
            fat32_copy_short_filename(i, filename, dir_data);
        }
        api(MODULE_API_PRINT, MODULE_NAME, "filename: %s\n", filename);
        if(!strcmp(path, filename)){
            // api(MODULE_API_PRINT, MODULE_NAME, "Found file: %s | Short: %s\n", filename, dir_data[i].name);
            *dirent_index = i;
            return &dir_data[i];
        }
        i++;
    }
    return 0;
}

fat_open_file_t *resolve_path(char *path, vfile_t *parent){
    const uint32_t MAX_TOKENS = PAGE_SIZE_BYTES/4;
    //resolve path and get cluster
    if(!parent){
        return 0;
    }
    if(!path || path[0] == 0){
        // api(MODULE_API_PRINT, MODULE_NAME, "Parent name: %s\n", parent->name);
        return 0;
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
    
    uint8_t err_in_search = 0;
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
    fat_dirent_t last_parent_dir = {0};
    fat_dirent_t parent_dir = {0};
    uint32_t dirent_index = 0;
    for(uint32_t i = 0; i < pathname_entries; i++){
        // api(MODULE_API_PRINT, MODULE_NAME, "Subpath: %s\n", path_tokens[i]);
        
        uint32_t size_to_alloc = parent_dir.name[0] ? (parent_dir.size + PAGE_SIZE_BYTES - 1) / PAGE_SIZE_BYTES : 8;
        size_to_alloc += (!size_to_alloc * 8);
        
        // api(MODULE_API_PRINT, MODULE_NAME, "Size to alloc: %d\n", size_to_alloc);
        fat_dirent_t *dir_data = malloc(api, size_to_alloc);
        
        // api(MODULE_API_PRINT, MODULE_NAME, "Parent->ID: %d\n", fat32_mounts[parent->id].mount_src->id);
        fat32_read_dirent(&parent_dir, (char *)dir_data, parent->id);
        
        fat_dirent_t *result = fat32_search_dir(path_tokens[i], dir_data, &dirent_index);
        
        if(!result){
            free(api, dir_data);
            err_in_search = 1;
            break;
        }
        last_parent_dir = parent_dir;
        parent_dir = *result;
        free(api, dir_data);
    }
    //check if a reference is open
    //return reference
    free(api, pathname);
    free(api, path_tokens);
    if(err_in_search){
        return 0;
    }
    fat_open_file_t *returnable = malloc(api, 1);
    if(!returnable){
        return 0;
    }
    uint32_t path_length = strlen(path);
    uint32_t copy_count = path_length >= 100 ? 100 : path_length;
    memcpy(path, returnable->filename, copy_count);
    
    returnable->first_cluster = (uint32_t)(parent_dir.cluster_high << 16) | parent_dir.cluster_low;
    if(!last_parent_dir.name[0]){
        returnable->dirent_cluster = fat32_mounts[parent->id].bpb->root_dir_cluster;
        returnable->dirent_offset = dirent_index;
    }
    else{
        returnable->dirent_cluster = ((uint32_t)(last_parent_dir.cluster_high << 16) | last_parent_dir.cluster_low);
        returnable->dirent_offset = dirent_index;
    }
    returnable->file_flags = parent_dir.flags;
    returnable->mount_index = parent->id;
    returnable->size_bytes = parent_dir.size;
    fat_mount_t const* mount = &fat32_mounts[parent->id];
    uint32_t cluster_size_bytes = mount->bpb->sectors_per_cluster * mount->bpb->bytes_per_sector;
    returnable->size_clusters = (parent_dir.size + cluster_size_bytes - 1)/cluster_size_bytes;
    
    //construct fat_open_file_t *and retur
    // puts(api, MODULE_NAME, "Returning!\n");
    return returnable;
}

vfile_t *fat32_open(char *path, vfile_t *parent){
    // puts(api, MODULE_NAME, "Called!\n");
    api(MODULE_API_PRINT, MODULE_NAME, "Path to open: %s\n", path);
    fat_open_file_t *file = resolve_path(path, parent);
    if(!file){
        if(!path[0]){
            file = malloc(api, 1);
            file->file_flags = parent->flags;
            file->dirent_cluster = 0;
            file->dirent_offset = 0;
            strcpy(parent->name, file->filename);
            api(MODULE_API_PRINT, MODULE_NAME, "Test mount id:%x\n", sizeof(fat_open_file_t));
            file->mount_index = parent->id;
            fat_mount_t mount = fat32_mounts[parent->id];
            file->refcount = 1;
            file->first_cluster = mount.bpb->root_dir_cluster;
            file->size_clusters = -1;
            file->size_bytes = 0;
        }else{
            puts(api, MODULE_NAME, "Error: File not found\n");
            return 0;
        }
    }
    vfile_t *to_return = malloc(api, 1);
    to_return->fileops = &fat32_fileops;
    to_return->flags = translate_fat32_flags_to_vfile(file->file_flags);
    to_return->id = file->mount_index;
    //cache entry
    to_return->offset = fat32_cache_open(file);
    to_return->private = file;
    to_return->block_size_bytes = 0;
    to_return->refcount = 1;
    to_return->size = file->size_bytes;
    // api(MODULE_API_PRINT, MODULE_NAME, "to return: %x\n", to_return);
    return to_return;
}

uint8_t lfn_checksum(const uint8_t sfn[11])
{
    uint8_t sum = 0;

    for (int i = 0; i < 11; i++)
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + sfn[i];

    return sum;
}

vfile_t *fat32_create(vfile_t *parent, char *path, FS_FILE_FLAGS flags){
    fat_mount_t *mount = &fat32_mounts[parent->id];
    
    uint32_t cluster_size_bytes = (mount->bpb->bytes_per_sector * mount->bpb->sectors_per_cluster);
    
    if(path[0] == 0){
        return 0;
    }
    //normally use ceiling division in the form of (n + x - 1)/x, but as i need to allocate space
    //for n + 1 bytes, the -1 cancels out
    char *trunc_path = malloc(api, (strlen(path) + PAGE_SIZE_BYTES )/PAGE_SIZE_BYTES);
    
    if(!trunc_path){
        return 0;
    }
    
    if(path[0] == '/') path++; //remove preceding '/'
    strcpy(path, trunc_path);
    char *file_to_create = trunc_path + strlen(trunc_path) - 1;
    if(file_to_create[0] == '/') file_to_create[0] = 0; //remove following '/' when creating dirs.
    
    while(*file_to_create != '/' && file_to_create > trunc_path){
        file_to_create--;
    }
    if(file_to_create[0] == '/'){
        //split string at '/' and set ptr to next char
        file_to_create[0] = 0;
        file_to_create++;
    }
    
    api(MODULE_API_PRINT, MODULE_NAME, "File to create: %s | Parent dir path: %s | Passed path: %s\n", file_to_create, trunc_path, path);
    
    vfile_t *parent_dir = parent;
    if(file_to_create != trunc_path){
        //if the file to create is not the entire path that has been passed, try and find the proper directory
        parent_dir = fat32_open(trunc_path, parent);
    }
    if(!parent_dir){
        api(MODULE_API_PRINT, MODULE_NAME, "Failed to create file: Could not find parent dir %s\n", trunc_path);
        free(api, trunc_path);
        return 0;
    }
    
    uint32_t filename_len = strlen(file_to_create);
    /*if(file_to_create == trunc_path)
    then trunc_path is the file to create, and parent is the correct parent directory
    */
    const uint32_t CHARS_PER_LFN = 13;
    uint32_t lfn_count = (filename_len + CHARS_PER_LFN - 1) / CHARS_PER_LFN;
    
    api(MODULE_API_PRINT, MODULE_NAME, "Total dirents required: %d\n", lfn_count);
    
    uint32_t cluster_count = 0;
    fat_open_file_t *parent_dir_of = parent_dir->private;
    if(!parent_dir_of){
        free(api, trunc_path);
        return 0;
    }
    uint32_t cluster = parent_dir_of->first_cluster;
    uint32_t last_cluster = 0;
    fat_dirent_t *buffer = 0;
    if(cluster == 0){
        cluster = fat32_get_free_cluster(parent->id);
        last_cluster = cluster;
        if(!cluster){
            free(api, trunc_path);
            return 0;
        }
        fat32_set_next_cluster(cluster, FAT32_EOC, parent->id);
        cluster_count = 1;
        
        buffer = malloc(api, (cluster_size_bytes + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES);
        if(!buffer){
            free(api, trunc_path);
            return 0;
        }
        for(uint32_t i = 0; i < cluster_size_bytes / sizeof(uint32_t); i++){
            ((uint32_t *)buffer)[i] = 0;
        }
    }
    else{
        while(cluster < FAT32_EOC){
            cluster_count++;
            last_cluster = cluster;
            cluster = fat32_get_next_cluster(cluster, parent->id);
        }
        api(MODULE_API_PRINT, MODULE_NAME, "Cluster count: %x\n", cluster_count);
        buffer = malloc(api, ((cluster_size_bytes * cluster_count) + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES);
        if(!buffer){
            free(api, trunc_path);
            return 0;
        }
        fat32_read(parent_dir, buffer, 0, cluster_count * cluster_size_bytes);
    }
    //11 is the number of characters in the 8.3 filename format
    
    // uint32_t needs_lfn = 1;
    // uint32_t ext_index = 0;
    
    
    // for(uint32_t i = 0; i < 9 && i < filename_len; i++){
    //     if(file_to_create[i] ==// '.'){
    //         needs_lfn = 0;
    //         ext_index = i;
    //         break;
    //     }
    // }
    // if((filename_len - (ext_index + 1)) > 3){
    //     needs_lfn = 1;
    // }
    uint32_t total_dirents = lfn_count + 1; //account for the dirent
    //if it doesn't need an LFN, then we can just use
    //the single dirent with the short filename
    uint32_t dir_entry_count = (cluster_size_bytes * cluster_count) / sizeof(fat_dirent_t);
    
    uint32_t found = 0;
    uint32_t free_index = (uint32_t)-1;
    char short_filename[20];
    uint32_t short_filename_matches = 0;
    memcpy(file_to_create, short_filename, 6);
    for(uint32_t i = 0; i < dir_entry_count; i++){
        found = 1;
        for(uint32_t j = 0; j < total_dirents; j++){
            if(i+j >= dir_entry_count){
                found = 0;
                break;
            }
            if(buffer[i+j].name[0] == 0){
                break;
            }
            if(buffer[i+j].name[0] != 0xe5){
                found = 0;
                i += j;
                break;
            }
        }
        if(buffer[i].name[0] == 0){
            found = 1;
            free_index = i;
            break;
        }
        if(buffer[i].name[0] == file_to_create[0]){
            short_filename_matches++;
        }
        if(found == 0){
            continue;
        }
        if(i < free_index){
            free_index = i;
        }
    }
    
    if(!found ||(free_index + total_dirents) > dir_entry_count){
        api(MODULE_API_PRINT, MODULE_NAME, "Directory full! extending...\n");
        uint32_t new_cluster = fat32_get_free_cluster(parent->id);
        fat32_set_next_cluster(new_cluster, FAT32_EOC, parent->id);
        if(!new_cluster){
            free(api, buffer);
            free(api, trunc_path);
            return 0;
        }
        fat32_set_next_cluster(last_cluster, new_cluster, parent->id);
        void *old_buffer = buffer;
        buffer = malloc(api, ((cluster_count + 1) * cluster_size_bytes + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES);
        memcpy(old_buffer, buffer, cluster_count++ * cluster_size_bytes);
        memclr(buffer + cluster_size_bytes * cluster_count, cluster_size_bytes);
        if(!found){
            free_index = dir_entry_count;
        }
        dir_entry_count += cluster_size_bytes/sizeof(fat_dirent_t);
        free(api, old_buffer);
    }
    
    api(MODULE_API_PRINT, MODULE_NAME, "Found free dirent at %x, needed %d consecutive\n", free_index, total_dirents);
    
    fat_dirent_t *dirents_to_write = malloc(api, (total_dirents + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES);
    uint32_t name_pos = 0;
    
    short_filename[0] = file_to_create[0];
    short_filename[1] = '~';
    //cannot fit in 6 digits, cannot make sfn
    //(why are there 100,000 files with the same sfn in the same directory???)
    if(short_filename_matches > 999999){
        free(api, buffer);
        free(api, trunc_path);
        free(api, dirents_to_write);
        return 0;
    }
    itoa(short_filename_matches, short_filename + 2, 10);
    
    
    uint32_t ext_index = 0;
    while(ext_index < filename_len && file_to_create[ext_index++] != '.');
    
    memcpy(file_to_create + ext_index, short_filename+8, 3);
    uint32_t checksum = lfn_checksum(short_filename);
    
    for(int i = 0; i < total_dirents; i++){
        if(i == total_dirents - 1){
            //the *real* dirent
            fat_dirent_t *dirent = &dirents_to_write[i];
            
            memcpy(short_filename, dirent->name, 11);
            
            dirent->flags = flags & 0x2f;
            dirent->size = 0;
            dirent->cluster_high = 0;
            dirent->cluster_low = 0;
            break;
        }
        fat_lfn_t *lfn_ent = (fat_lfn_t*)(&dirents_to_write[i]);
        *lfn_ent = (fat_lfn_t){0};
        api(MODULE_API_PRINT, MODULE_NAME, "Making lfn entry: %d in series of %d\n", i, total_dirents);
        for(uint32_t j = 0; j < 5 && file_to_create[name_pos]; j++){
            lfn_ent->name0[j] = file_to_create[name_pos++];
        }
        for(uint32_t j = 0; j < 6 && file_to_create[name_pos]; j++){
            lfn_ent->name1[j] = file_to_create[name_pos++];
        }
        for(uint32_t j = 0; j < 2 && file_to_create[name_pos]; j++){
            lfn_ent->name2[j] = file_to_create[name_pos++];
        }
        char tb[33] = {0};
        for(uint32_t i = 0; i < 32; i++){
            tb[i] = ((char*)lfn_ent)[i];
            if(tb[i] == 0) tb[i] = 0x20;
        }
        api(MODULE_API_PRINT, MODULE_NAME, "Test: %s, name pos: %x\n", tb, sizeof(fat_lfn_t));
        lfn_ent->entry_no = i + 1;
        lfn_ent->checksum = checksum;
        lfn_ent->attribute = 0xf;
        if(i == total_dirents - 2){
            api(MODULE_API_PRINT, MODULE_NAME, "Final filename entry!\n");
            lfn_ent->entry_no |= 0x40;
        }
    }
    for(uint32_t i = 0; i < total_dirents; i++){
        if(i == total_dirents - 1){
            //write final dirent
            buffer[i + free_index] = dirents_to_write[total_dirents - 1];
            api(MODULE_API_PRINT, MODULE_NAME, "Pos: %x\n", total_dirents - 1);
            break;
        }
        api(MODULE_API_PRINT, MODULE_NAME, "Pos: %x\n", total_dirents - 1 - i);
        buffer[i + free_index] = dirents_to_write[total_dirents - 2 - i];
    }
    fat32_write(parent_dir, buffer, 0, cluster_size_bytes * cluster_count);
    
    free(api, trunc_path);
    free(api, buffer);
    free(api, dirents_to_write);
    
    return fat32_open(path, parent);
}

int fat32_delete(vfile_t *file, char *child){
    
}

int dirent_writeback(vfile_t *file){
    fat_open_file_t *open_file = file->private;
    fat_mount_t *mount = &fat32_mounts[open_file->mount_index];
    uint32_t cluster_size_bytes = (mount->bpb->bytes_per_sector * mount->bpb->sectors_per_cluster);

    size_t dirents_per_cluster = cluster_size_bytes/sizeof(fat_dirent_t);
    uint32_t dirent_index = open_file->dirent_offset;
    api(MODULE_API_PRINT, MODULE_NAME, "Dirent Cluster: %x, Index: %x, Dirent cluster offset from root cluster: %x, Total Dirents per cluster: %x\n", open_file->dirent_cluster, dirent_index, dirent_index/dirents_per_cluster, dirents_per_cluster);
    
    
    uint32_t cluster = open_file->dirent_cluster;
    for(uint32_t i = 0; i < dirent_index/dirents_per_cluster; i++){
        if(cluster >= FAT32_EOC && i < (dirent_index/dirents_per_cluster - 1)){
            puts(api, MODULE_NAME, "Error: Reached end of cluster chain before reaching dirent's cluster\n");
            api(MODULE_API_PRINT, MODULE_NAME, "Cluster Index: %x, Dirent Index: %x, #%d in chain\n", dirent_index/dirents_per_cluster, dirent_index, i);
            puts(api, MODULE_NAME, "Data is malformed, Or memory corruption has occured (or there's a bug in the code). Please report this bug at https://github.com/raeofsunshinedev/kimisos \n");
            return -1;
        }
        cluster = fat32_get_next_cluster(cluster, open_file->mount_index);
    }
    api(MODULE_API_PRINT, MODULE_NAME, "Cluster: %x\n", cluster);
    fat_dirent_t *dir = malloc(api, 1);
    uint64_t cluster_offset_bytes = ((cluster - 2) * cluster_size_bytes) + mount->data_start_sector * mount->bpb->bytes_per_sector;
    fread(api, mount->mount_src, dir, cluster_offset_bytes, PAGE_SIZE_BYTES);
    uint32_t index_adj = dirent_index % dirents_per_cluster;
    
    dir[index_adj].size = open_file->size_bytes;
    dir[index_adj].cluster_high = (open_file->first_cluster >> 16);
    dir[index_adj].cluster_low = (open_file->first_cluster & 0xffff);
    dir[index_adj].flags = open_file->file_flags;
    //TODO: Put creation time in here (eventually)
    fwrite(api, mount->mount_src, dir, cluster_offset_bytes, PAGE_SIZE_BYTES);
    
}

int fat32_write(vfile_t *file, void *buffer, uint64_t offset, uint64_t count){
    puts(api, MODULE_NAME, "write called!\n");
    fat_open_file_t *open_file = file->private;
    fat_mount_t *mount = &fat32_mounts[open_file->mount_index];
    uint32_t first_cluster = open_file->first_cluster;
    if(first_cluster == 0){
        first_cluster = fat32_get_free_cluster(file->id);
        if(!first_cluster){
            return 0;
        }
        fat32_set_next_cluster(first_cluster, FAT32_EOC, file->id);
        open_file->first_cluster = first_cluster;
    }
    uint32_t cluster_size_bytes = (mount->bpb->bytes_per_sector * mount->bpb->sectors_per_cluster);
#ifdef __i386__
    // api(MODULE_API_PRINT, MODULE_NAME, "Test %x\n", udiv64(8, 2));
    uint32_t clusters_until_start = udiv64(offset, cluster_size_bytes);
    uint32_t clusters_to_write = udiv64(count + cluster_size_bytes - 1, cluster_size_bytes);
#else
    uint32_t clusters_until_start = offset / cluster_size_bytes;
    uint32_t clusters_to_write = (count + cluster_size_bytes - 1)/ cluster_size_bytes;
#endif
    api(MODULE_API_PRINT, MODULE_NAME, "File name: %s | First cluster: %x | Clusters until start: %x | Clusters to write: %x\n", open_file->filename, first_cluster, clusters_until_start, clusters_to_write);
    uint32_t current_cluster = first_cluster;
    uint32_t last_cluster = 0;
    // uint32_t free = fat32_get_free_cluster(open_file->mount_index);
    for(int i = 0; i < clusters_until_start; i++){
        // api(MODULE_API_PRINT, MODULE_NAME, "Getting cluster: %x\n", free);
        last_cluster = current_cluster;
        current_cluster = fat32_get_next_cluster(current_cluster, open_file->mount_index);
        if(current_cluster >= FAT32_EOC_MIN){
            spinlock_acquire(&(mount->spinlock));
            uint32_t new_cluster = fat32_get_free_cluster(open_file->mount_index);
            if(!new_cluster){
                api(MODULE_API_PRINT, MODULE_NAME, "Failed to allocate new cluster!\n");
                spinlock_release(&(mount->spinlock));
                return 0;
            }
            fat32_set_next_cluster(last_cluster, new_cluster, open_file->mount_index);
            fat32_set_next_cluster(new_cluster, FAT32_EOC, open_file->mount_index);
            spinlock_release(&(mount->spinlock));
            //write zeros to the newly allocated cluster;
            uint32_t *cleared = malloc(api, 1);
            if(!cleared){
                return 0;
            }
            for(uint32_t j = 0; j < PAGE_SIZE_BYTES / sizeof(uint32_t); j++){
                cleared[j] = 0;
            }
            free(api, cleared);
            uint64_t cluster_offset_bytes = ((new_cluster - 2) * cluster_size_bytes);
            uint64_t write_offset = cluster_offset_bytes + mount->data_start_sector * mount->bpb->bytes_per_sector;
            fwrite(api, mount->mount_src, (char *)cleared, write_offset, PAGE_SIZE_BYTES);
            current_cluster = new_cluster;
        }
    }
    
    uint64_t remaining_count = count;
    
    if(offset & (cluster_size_bytes - 1)){
        uint32_t offset_in_cluster = offset & (cluster_size_bytes - 1);
        uint64_t cluster_offset_bytes = ((current_cluster - 2) * cluster_size_bytes);
        uint64_t write_offset = cluster_offset_bytes + mount->data_start_sector * mount->bpb->bytes_per_sector;
        fwrite(api, mount->mount_src, buffer, write_offset + offset_in_cluster, ((cluster_size_bytes - offset_in_cluster) > count ? count : cluster_size_bytes - offset_in_cluster));
        if((cluster_size_bytes - offset_in_cluster) > count){
            return count;
        }
        buffer += cluster_size_bytes - offset_in_cluster;
        current_cluster = fat32_get_next_cluster(current_cluster, open_file->mount_index);
        remaining_count -= (cluster_size_bytes - offset_in_cluster);
        if(current_cluster >= FAT32_EOC_MIN){
            return count - remaining_count;
        }
    }
    uint32_t i = 0;
    while(remaining_count > 0){
        // api(MODULE_API_PRINT, MODULE_NAME, "Cluster: %x\n", current_cluster);
        uint64_t cluster_offset_bytes = ((current_cluster - 2) * cluster_size_bytes);
        uint64_t write_offset = cluster_offset_bytes + mount->data_start_sector * mount->bpb->bytes_per_sector;
        if((remaining_count < cluster_size_bytes)){
            fwrite(api, mount->mount_src, buffer + i * cluster_size_bytes, write_offset, remaining_count);
            remaining_count = 0;
        }
        else{
            fwrite(api, mount->mount_src, buffer + i * cluster_size_bytes, write_offset, cluster_size_bytes);
            remaining_count -= cluster_size_bytes;
        }
        
        last_cluster = current_cluster;
        current_cluster = fat32_get_next_cluster(current_cluster, open_file->mount_index);
        if(current_cluster >= FAT32_EOC_MIN && remaining_count > 0){
            spinlock_acquire(&(mount->spinlock));
            uint32_t new_cluster = fat32_get_free_cluster(open_file->mount_index);
            
            if(!new_cluster){
                spinlock_release(&(mount->spinlock));
                api(MODULE_API_PRINT, MODULE_NAME, "Failed to allocate new cluster!\n");
                return i * cluster_size_bytes;
            }
            fat32_set_next_cluster(last_cluster, new_cluster, open_file->mount_index);
            fat32_set_next_cluster(new_cluster, FAT32_EOC, open_file->mount_index);
            spinlock_release(&(mount->spinlock));
            current_cluster = new_cluster;
        }
        i++;
    }
    if((offset + count) > open_file_cache[file->offset]->size_bytes){
        open_file_cache[file->offset]->size_bytes = offset + count;
    }
    dirent_writeback(file);
    return count;
}

int fat32_read(vfile_t *file, void *buffer, uint64_t offset, uint64_t count){
    // puts(api, MODULE_NAME, "Read called!\n");
    fat_open_file_t *open_file = file->private;
    fat_mount_t *mount = &fat32_mounts[open_file->mount_index];
    uint32_t first_cluster = open_file->first_cluster;
    uint32_t cluster_size_bytes = (mount->bpb->bytes_per_sector * mount->bpb->sectors_per_cluster);
#ifdef __i386__
    // api(MODULE_API_PRINT, MODULE_NAME, "Test %x\n", udiv64(8, 2));
    uint32_t clusters_until_start = udiv64(offset, cluster_size_bytes);
    uint32_t clusters_to_read = udiv64(count + cluster_size_bytes - 1, cluster_size_bytes);
#else
    uint32_t clusters_until_start = offset / cluster_size_bytes;
    uint32_t clusters_to_read = (count + cluster_size_bytes - 1)/ cluster_size_bytes;
#endif
    // api(MODULE_API_PRINT, MODULE_NAME, "File name: %s | First cluster: %x | Clusters until start: %x | Clusters to read: %x\n", open_file->filename, first_cluster, clusters_until_start, clusters_to_read);
    uint32_t current_cluster = first_cluster;
    for(int i = 0; i < clusters_until_start; i++){
        // api(MODULE_API_PRINT, MODULE_NAME, "Getting cluster: %x\n", )
        current_cluster = fat32_get_next_cluster(current_cluster, open_file->mount_index);
        if(current_cluster >= FAT32_EOC_MIN){
            return 0;
        }
    }
    
    uint64_t remaining_count = count;
    
    if(offset & (cluster_size_bytes - 1)){
        uint32_t offset_in_cluster = offset & (cluster_size_bytes - 1);
        uint64_t cluster_offset_bytes = ((current_cluster - 2) * cluster_size_bytes);
        uint64_t read_offset = cluster_offset_bytes + mount->data_start_sector * mount->bpb->bytes_per_sector;
        fread(api, mount->mount_src, buffer, read_offset + offset_in_cluster, ((cluster_size_bytes - offset_in_cluster) > count ? count : cluster_size_bytes - offset_in_cluster));
        if((cluster_size_bytes - offset_in_cluster) > count){
            return count;
        }
        buffer += cluster_size_bytes - offset_in_cluster;
        current_cluster = fat32_get_next_cluster(current_cluster, open_file->mount_index);
        remaining_count -= (cluster_size_bytes - offset_in_cluster);
        if(current_cluster >= FAT32_EOC_MIN){
            return count - remaining_count;
        }
    }
    // api(MODULE_API_PRINT, MODULE_NAME, "Starting on cluster: %x\n", current_cluster);
    uint32_t i = 0;
    while(remaining_count > 0){
        uint64_t cluster_offset_bytes = ((current_cluster - 2) * cluster_size_bytes);
        uint64_t read_offset = cluster_offset_bytes + mount->data_start_sector * mount->bpb->bytes_per_sector;
        uint32_t offset_in_cluster = 0;
        api(MODULE_API_PRINT, MODULE_NAME, "Reading cluster: %x, offset at %x\n", current_cluster, read_offset);
        if((remaining_count < cluster_size_bytes)){
            puts(api, MODULE_NAME, "Last Cluster!\n");
            // fread(api, mount->mount_src, buffer + i * cluster_size_bytes, read_offset + (offset & (cluster_size_bytes - 1)), count & (cluster_size_bytes - 1) - (offset & (cluster_size_bytes - 1)));
            fread(api, mount->mount_src, buffer + i * cluster_size_bytes, read_offset, remaining_count);
            remaining_count = 0;
        }
        else{
            fread(api, mount->mount_src, buffer + i * cluster_size_bytes, read_offset, cluster_size_bytes);
            remaining_count -= cluster_size_bytes;
        }
        current_cluster = fat32_get_next_cluster(current_cluster, open_file->mount_index);
        if(current_cluster >= FAT32_EOC_MIN && i < (clusters_to_read - 1)){
            return (i + 1) * cluster_size_bytes;
        }
        i++;
    }
    return count;
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
        puts(api, MODULE_NAME, "Failed to create mountfile\n");
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
    mountfile->block_size_bytes = bpb->bytes_per_sector * bpb->sectors_per_cluster;
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