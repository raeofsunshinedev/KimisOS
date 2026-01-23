#pragma once
#include <stdint.h>

void idt_load();
typedef struct cpu_registers{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; 
    uint32_t int_no, pfa;
    uint32_t eip, cs, eflags, useresp, ss;
}__attribute__((packed))cpu_registers_t;
typedef struct idt{
    uint16_t size;
    uint32_t pointer;
}__attribute__((packed))idt_t;

#define IDT_GATE_TASK   0x5
#define IDT_GATE_INT16  0x6
#define IDT_GATE_TRAP16 0x7
#define IDT_GATE_INT    0xe
#define IDT_GATE_TRAP   0xf
#define IDT_DPL_KERNEL  0 << 5
#define IDT_DPL_USER    3 << 5

cpu_registers_t *syscall(cpu_registers_t * regs);

inline void enable_interrupts(){
    asm volatile("sti");
}
inline void disable_interrupts(){
    asm volatile("cli");
}
void kernel_panic(char *msg, cpu_registers_t *regs);
void install_irq_handler(void (*handler)(), uint8_t irqno);
void *get_irq_handler(uint8_t irqno);
uint8_t get_in_interrupt();