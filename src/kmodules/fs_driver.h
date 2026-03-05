#pragma once
#include <stdint.h>

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