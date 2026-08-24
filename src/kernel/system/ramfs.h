#pragma once
#include <stdint.h>

void ramfs_init();
vfile_t *ramfs_create(vfile_t *parent, char *path, FS_FILE_FLAGS flags);
//Will never create a new reference
vfile_t *get_root_dir();

int ramfs_delete(vfile_t *parent, char *child);
int ramfs_write(vfile_t *file, char *buffer, uint64_t offset, uint64_t count);
int ramfs_read(vfile_t *file, char *buffer, uint64_t offset, uint64_t count);
// vfile_t *ramfs_open(char *path);
void ramfs_close(vfile_t *file);
vfile_t *ramfs_rfopen(char *name, vfile_t *parent);