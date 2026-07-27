#include "../system/vfs.h"
#include <stdint.h>

typedef struct kernel_config_s{
    uint32_t kernel_heap_size;
    vfile_t *logfile;
    uint32_t user_stack_size;
    uint32_t swap_size;
    vfile_t *swapfile;
    uint8_t swap_enable;
    uint8_t panic_on_exception;
}kernel_config_t;

void config_init();