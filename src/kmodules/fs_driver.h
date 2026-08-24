#pragma once
#include <stdint.h>

#define FAT32_LONG_FILE_NAME 0xF
#define FAT32_READ_ONLY 0x1
#define FAT32_HIDDEN 0x2
#define FAT32_SYSTEM 0x4
#define FAT32_VOLUME_ID 0x8
#define FAT32_DIRECTORY 0x10
#define FAT32_ARCHIVE 0x20

typedef struct fat32_bpb{
    //fat12/fat16 bpb
    char nop[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_dir_entries;
    uint16_t sectors_small;
    uint8_t media;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t partition_start;
    uint32_t sector_count;
    //fat32 exclusive
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_dir_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_bpb_sector;
    uint8_t reserved[12];
    uint8_t drive_no;
    uint8_t ntflags;
    uint8_t signature; //must be 0x28 or 0x29
    uint32_t serial_id;
    char label[11];
    char system_id[8];
    char boot_code[420];
    uint16_t bootable_sig;
}__attribute__((packed)) fat32_bpb_t;

#define FAT32_FSINFO_LEAD_SIG 0x41615252
#define FAT32_FSINFO_SIG2 0x61417272
#define FAT32_FSINFO_TRAIL_SIG 0xAA550000
typedef struct fsinfo_s{
    uint32_t lead_sig;
    uint8_t reserved[480];
    uint32_t sig2;
    uint32_t last_free_cluster_count;
    uint32_t first_free_cluster;
    uint8_t reserved_2[12];
    uint32_t trail_sig;
}fsinfo_t;

typedef struct fat_mount_s{
    fat32_bpb_t *bpb;
    vfile_t *mount_src;
    
    struct fat_mount_flags{
        uint8_t cache_dirty:1;
        uint8_t read_only:1;
        uint8_t fs_info_dirty:1;
    } flags;
    
    spinlock_t *spinlock;
    
    uint32_t last_free_cluster_count;
    uint32_t fat_search_start;
    uint32_t max_clusters;
    uint32_t fat_start_sector;
    uint32_t data_start_sector;
    
    uint32_t fat_cache_start;
    uint32_t fat_cache_size; //in entries
    uint32_t *fat_cache;
} __attribute__((aligned(64))) fat_mount_t;

typedef struct fat_open_file_s{
    char filename[100];
    
    uint32_t refcount;
    
    uint32_t mount_index;
    uint32_t first_cluster;
    uint32_t size_clusters;
    
    uint32_t file_flags;
    
    //first cluster of dirent;
    uint32_t dirent_cluster;
    //index as array of fat32 dirents
    uint32_t dirent_offset;
}fat_open_file_t;

typedef struct fat_dirent_s{
    char name[11];
    uint8_t flags;
    uint8_t reserved;
    uint8_t creation_hundreths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_accessed_date;
    uint16_t cluster_high;
    uint16_t last_modification_time;
    uint16_t last_modification_date;
    uint16_t cluster_low;
    uint32_t size;
}__attribute__((packed)) fat_dirent_t;

typedef struct fat_lfn_s{
    uint8_t entry_no;
    uint16_t name0[5];
    uint8_t attribute;
    uint8_t zero;
    uint8_t checksum;
    uint16_t name1[6];
    uint16_t zero1;
    uint16_t name2[2];
}__attribute__((packed)) fat_lfn_t;

int fat32_read(vfile_t *file, void *buffer, uint64_t offset, uint64_t count);
int fat32_write(vfile_t *file, void *buffer, uint64_t offset, uint64_t count);