#pragma once
#include "modlib.h"
#include <stdint.h>

#define PARTITION_BOOTABLE 0x80

typedef struct partition_entry_s{
    vfile_t *file;
    uint32_t start_lba;
    uint32_t sector_count;
    uint32_t flags; //unused for now
}partent_t;

typedef struct partition_reference_s{
    uint32_t part_count;
    vfile_t *parent;
    uint32_t disk_flags;
    uint32_t reserved;
    
    partent_t partitions[127];
}pref_t;

typedef struct partition_table_entry_s{
    uint8_t flags;
    uint8_t chs_address[3];
    uint8_t type;
    uint8_t chs_address_end[3];
    uint32_t lba_address;
    uint32_t sector_count;
}__attribute__((packed))partition_table_entry_t;

typedef struct mbr_s{
    uint8_t boot_code[440];
    uint32_t disk_id;
    uint16_t reserved;
    partition_table_entry_t partition_table[4];
    uint16_t sig;
}__attribute__((packed))mbr_t;