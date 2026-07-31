#pragma once
#include "modlib.h"
#include <stdint.h>

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