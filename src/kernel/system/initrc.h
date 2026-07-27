#pragma once
#include <stdint.h>
#include "vfs.h"

void initrc_read(vfile_t *file);


typedef struct config_map_entry{
    char *name;
    void (*handler)(uint32_t value);
}config_map_entry_t;

void config_set_kernel_heap_size(uint32_t value);