#pragma once
#include <stdint.h>

void ramfs_init();
void ramfs_create(char *path, FS_FILE_FLAGS flags);
vfile_t *get_root_dir();