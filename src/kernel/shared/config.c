#include "config.h"

kernel_config_t config;

void config_init(){
    config.kernel_heap_size = 64 << 20;
    config.logfile = 0;
    config.user_stack_size = 1 << 20;
    config.swap_enable = 0;
    config.panic_on_exception = 1;
}

kernel_config_t *get_config(){
    return &config;
}

void config_set_kernel_heap_size(uint32_t value){
    config.kernel_heap_size = value;
}
void config_set_user_stack_size(uint32_t value){
    config.user_stack_size = value;
}
void config_set_swap_enable(uint32_t value){
    config.swap_enable = value;
}
void config_set_expanic(uint32_t value){
    config.panic_on_exception = value;
}
void config_set_swap_size(uint32_t value){
    config.swap_size = value;
}