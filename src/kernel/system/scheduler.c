#include "../shared/interrupts.h"
#include "scheduler.h"
#include "../shared/string.h"
#include "../shared/kstdlib.h"
#include "../shared/memory.h"

volatile process_t *processes;
volatile uint32_t process_queue[PROCESS_COUNT];
uint32_t volatile active_processes;
uint32_t volatile current_pid;
uint32_t volatile queue_length;
uint32_t volatile current_queue_index;
void scheduler_init(){
    // printf("?");
    // process_queue = kmalloc((PROCESS_COUNT * sizeof(uint32_t)) / 4096);
    processes = kmalloc((PROCESS_COUNT * sizeof(process_t) + 4095) / 4096);
    // process_queue = kmalloc(1);
    // printf("%x, %x\n", (PROCESS_COUNT * sizeof(process_t)) / 4096, (PROCESS_COUNT * sizeof(uint32_t)) / 4096);
    // printf("%x, %x\n", processes, process_queue);
    for(uint32_t i = 0; i < PROCESS_COUNT; i++){
        // printf("Index: %x, %d\n", process_queue, sizeof(process_t));
        processes[i].flags.present = 0;
        process_queue[i] = 0;
    }
    current_pid = 0;
    queue_length = 0;
    current_queue_index = 0;
    active_processes = 0;
    // printf("Init :3");
    install_irq_handler(schedule, 0);
    // printf("Installing scheduler");
}
uint32_t timer = 0;

void add_process_queue(uint32_t pid){
    asm("cli");
    asm volatile ("" : : :"memory");
    process_queue[queue_length] = pid;
    queue_length++;
    // printf("adding %d @ %d", pid, queue_length-1);
    // printf("queue: %x", queue_length);
    asm("sti");
    return;
}
//Watch out for this function.
void remove_process_queue(uint32_t pid){
    asm("cli");
    uint32_t last_queue_index = queue_length-1;
    uint32_t old_index = -1;
    for(uint32_t i = 0; i <= last_queue_index; i++){
        if(process_queue[i] == pid){
            old_index = i;
            process_queue[i] = 0;
        }
    }
    if(old_index == last_queue_index || old_index < queue_length){
        asm("sti");
        return;
    }
    process_queue[old_index] = process_queue[last_queue_index];
    process_queue[last_queue_index] = 0;
    queue_length--;
    if(current_queue_index > 0)
        current_queue_index--;
    asm("sti");
}

cpu_registers_t *schedule(cpu_registers_t *regs){
    if(queue_length == 0){
        add_process_queue(0);
        // return regs;
    }
    // uint32_t i = 0;
    processes[current_pid].cpuregs = *regs;
    current_queue_index++;
    if(current_queue_index >= queue_length){
        current_queue_index = 0;
    }
    // current_pid = [current_queue_index]
    current_pid = process_queue[current_queue_index];
    asm volatile("mov %0, %%cr3" : : "r"(processes[current_pid].page_dir));
    // printf("pid: %d, %d\n", run_count, queue_length);
    cpu_registers_t *newregs = (cpu_registers_t*)processes[current_pid].cpuregs.esp;
    // printf("EAX: %x EBX: %x ECX: %x EDX: %x\n", newregs->eax, newregs->ebx, newregs->ecx, newregs->edx);
    // printf("ESI: %x EDI: %x ESP: %x EBP: %x\n", newregs->esi, newregs->edi, newregs->esp, newregs->ebp);
    // printf("EIP: %x CS: %x DS: %x\n", newregs->eip, newregs->cs, newregs->ds);
    return newregs;
}

uint32_t spawn_new_process(cpu_registers_t defaultregs, char **argv, uint32_t argc, void *cr3){
    uint32_t i = current_pid;
    while(processes[i].flags.present){
        i++;
        if(i >= PROCESS_COUNT) i = 0;
        if(i == current_pid) return -1;
    }
    // i += !active_processes;
    active_processes++;
    process_t new_proc = {0};
    new_proc.argc = argc;
    new_proc.argv = argv;
    new_proc.page_dir = cr3;
    new_proc.cpuregs = defaultregs;
    new_proc.parent = current_pid;
    new_proc.flags.present = 1;
    new_proc.flags.cpu_lvl = 1;
    new_proc.flags.priority = 0;
    new_proc.flags.system = processes[current_pid].flags.system;
    processes[i] = new_proc;
    add_process_queue(i);
    return i;
}

void fork(){
    //make a copy of original process, create new address space,
    //and copy all page entries with write disabled
    //(so we con perform a copy on write)
}
void exec(char *filename, char **argv){
    //open and read file {filename}
    //then create a new address space, parse elf header and create
    //a new schedulable entity
}
void kill(uint32_t pid){
    void *pd = processes[pid].page_dir;
    uint32_t *pd_ptr = kmalloc(1);
    uint32_t *pt_ptr = kmalloc(1);
    kfree(pt_ptr);
    kfree(pd_ptr);
    map(pd_ptr, pd, PT_PRESENT);
    for(uint32_t i = 0; i < 1024; i++){
        map(pt_ptr, (void *)pd_ptr[i], PT_PRESENT);
        for(uint32_t j = 0; j < 1024; j++){
            uint32_t pt_paddr = pt_ptr[j] & ~(0xfff);
            pm_free(pt_paddr);
        }
        unmap(pt_ptr);
    }
    processes[pid].flags.present = 0;
    processes[pid].page_dir = 0;
    remove_process_queue(pid);
}

void set_pid_blocked(uint32_t pid){
    processes[pid].flags.blocked = 1;
    remove_process_queue(pid);
}
void set_pid_unblocked(uint32_t pid){
    processes[pid].flags.blocked = 0;
    add_process_queue(pid);
}
uint32_t get_current_pid(){
    return current_pid;
}
uint32_t thread_start(void (*function)()){
    cpu_registers_t regs = {0};
    regs.cs = 0x10;
    regs.ds = 0x18;
    regs.es = 0x18;
    regs.ss = 0x18;
    regs.eflags = 0x202;
    // regs.ebp = 
    regs.esp = (uint32_t)kmalloc(8) - sizeof(regs) + 8;
    regs.ebp = regs.esp;
    regs.eip = (uint32_t)function;
    memcpy((char *)&regs, (char*)regs.esp, sizeof(regs));
    uint32_t retpid = spawn_new_process(regs, 0, 0, (void *)0x10000);
    processes[retpid].parent = current_pid;
    // printf("returning :3");
    // for(;;);
    return retpid;
}
void thread_join(uint32_t thread_id, uint32_t *exit_code){
    asm volatile ("" : : :"memory");
    // while(processes[thread_id].flags.present){
    //     // printf("%d", processes[thread_id].flags.present);
    // };
    // printf("Done");
    set_pid_blocked(current_pid);
    asm("int $32");
    *exit_code = processes[thread_id].exit_value;
}
void thread_exit(uint32_t exit_code){
    processes[current_pid].flags.present = 0;
    processes[current_pid].exit_value = exit_code;
    remove_process_queue(current_pid);
    set_pid_unblocked(processes[current_pid].parent);
    // printf("Exiting pid %d, %d\n", current_pid);
    asm volatile ("int $32");//call scheduler via interrupt
}
