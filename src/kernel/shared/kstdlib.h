#pragma once
// #include "memory.h"
#include "stdint.h"
#include "../drivers/serial.h"
#include "interrupts.h"

#define MLOG_DEBUG 1
#define MLOG_PRINT 2
#define MLOG_WARN 3
#define MLOG_ERR 4

typedef struct initrd{
    void *ptr;
    uint32_t size;
}__attribute__((packed)) initrd_t;

typedef struct vector{
    void *ptr;
    uint32_t size;
    uint32_t sizeof_elements;
    uint32_t pages;
}vector_t;

vector_t *init_vector(void *vecptr, uint32_t sizeof_elements, uint32_t elements, void *initializer);
void *vector_get(uint32_t pos, vector_t *vector);
void vector_set(uint32_t pos, vector_t *vector, void *new_element);
void vector_push(vector_t *vector, void *new_element);
void vector_pop(uint32_t pos, vector_t *vector, void *element);

typedef struct kernel_info{
    void *mmap_ptr;
    uint16_t mmap_entry_count;
    uint8_t padding;
    uint8_t pixel_depth;
    uint16_t monitor_x_resolution;
    uint16_t monitor_y_resolution;
    void *framebuffer;
    initrd_t *initrd;
    // uint32_t loaded_module_count;
}__attribute__((packed)) kernel_info_t;

void mlog(char *module, char *str, uint32_t type, ...);
void vmlog(char *module, char *str, uint32_t type, va_list vars);


#define MODULE_TYPE_DISK 1
#define MODULE_TYPE_FS 2
#define MODULE_TYPE_EXT 3
#define MODULE_TYPE_VIDEO 4

#define PANIC(m) kernel_panic(m, 0)