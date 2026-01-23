#pragma once
#include <stdint.h>
#include "../shared/interrupts.h"
typedef struct{
    char **argv;
    uint32_t argc;
    cpu_registers_t cpuregs;
    void *page_dir;
    struct pflags{
        uint8_t blocked :1;
        uint8_t system  :1;
        uint8_t cpu_lvl :1;
        uint8_t priority:3;
        uint8_t present :1;
        uint8_t ran     :1;
    }flags;
    uint32_t exit_value;
    uint32_t thread_id;
    uint32_t parent;
    uint32_t uid;
    uint32_t gid;
}process_t;

#define PROCESS_COUNT 0x10000

cpu_registers_t* schedule(cpu_registers_t *regs);
void scheduler_init();
uint32_t thread_start(void (*function)());
void thread_join(uint32_t thread_id, uint32_t *exit_code);
void thread_exit(uint32_t exit_code);
void set_pid_blocked(uint32_t pid);
void set_pid_unblocked(uint32_t pid);
uint32_t get_current_pid();