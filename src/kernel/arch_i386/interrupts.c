#include "../shared/interrupts.h"
#include "../drivers/serial.h"
#include "../drivers/cpuio.h"
#include <stdint.h>

uint8_t in_interrupt = 0;

uint8_t get_in_interrupt(){
    return in_interrupt;
}

extern void _isr0();
extern void _isr1();
extern void _isr2();
extern void _isr3();
extern void _isr4();
extern void _isr5();
extern void _isr6();
extern void _isr7();
extern void _isr8();
extern void _isr9();
extern void _isr10();
extern void _isr11();
extern void _isr12();
extern void _isr13();
extern void _isr14();
extern void _isr15();
extern void _isr16();
extern void _isr17();
extern void _isr18();
extern void _isr19();
extern void _isr20();
extern void _isr21();
extern void _isr22();
extern void _isr23();
extern void _isr24();
extern void _isr25();
extern void _isr26();
extern void _isr27();
extern void _isr28();
extern void _isr29();
extern void _isr30();
extern void _isr31();


extern void _irq0();
extern void _irq1();
extern void _syscall();
extern void load_idt(void*);
typedef struct idt_entry{
    uint16_t offset_low;
    uint16_t code_seg;
    uint8_t reserved;
    uint8_t flags;
    uint16_t offset_high;
}__attribute__((packed)) idt_entry_t;

cpu_registers_t *(*volatile interrupt_handlers[16])(cpu_registers_t *regs) = {0};
idt_t idt_desc;

idt_entry_t idt_table[256];

inline void set_idt_entry(uint32_t index, void *ptr, uint8_t flags, uint16_t segment){
    uint32_t offset = (uint32_t) ptr;
    idt_table[index].offset_high = offset >> 16;
    idt_table[index].offset_low = offset & 0xffff;
    idt_table[index].code_seg = segment;
    idt_table[index].flags = flags | 0x80;
    // idt_table[index] = (segment << 16) | (offset & 0xffff);
    // idt_table[index+1] = (offset >> 16) | ((flags | 0x80) << 8);
    // idt_table[index] = ((uint64_t)(offset >> 16) << 48) | ((uint64_t)(flags | 0x80) << 40) | (segment << 16) | (offset & 0xffff);
}


cpu_registers_t *_irq_handler(cpu_registers_t *regs){
    in_interrupt = 1;
    regs->esp = (uint32_t)regs;
    if(regs->int_no == 0x80){
        regs = syscall(regs);
    }
    else if(interrupt_handlers[regs->int_no - 32])
    {
        // printf("found one for an int");
        regs = interrupt_handlers[regs->int_no - 32](regs);
        // printf("returned;")
        // printf("eip: %x", regs->eip);
    }
    outb(0x20, 0x20);
    if(regs->int_no >= 0x28){
        outb(0xa0, 0x20);
    }
    in_interrupt = 0;
    return regs;
}

char *isr_error_strings[] = {"DE", "DB", "NMI", "BP", "OF", "BR", "UD", "NM", "DF", "CSO", "TS", "NP", "SS", "GP", "PF", "", "MF", "AC", "MC", "XM", "VE", "CP", ""};

void _isr_handler(cpu_registers_t *regs){
    printf("Kernel Panic!\n");
    printf("ISR: #%s\n", isr_error_strings[regs->int_no]);
    printf("EAX: %x EBX: %x ECX: %x EDX: %x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf("ESI: %x EDI: %x ESP: %x EBP: %x\n", regs->esi, regs->edi, regs->esp, regs->ebp);
    printf("EIP: %x CS: %x DS: %x SS: %x\n", regs->eip, regs->cs, regs->ds, regs->ss);
    printf("Error Code: %x\n", regs->pfa);
    if (regs->int_no == 14){
        uint32_t cr2;
        asm volatile("mov %%cr2, %%eax;mov %0, %%eax": "=r"(cr2));
        printf("Address: %x", cr2);
    }
    for(;;);
}

cpu_registers_t *syscall(cpu_registers_t *regs){
    
}

extern void panic_hold(cpu_registers_t *regs);

void kernel_panic(char *message, cpu_registers_t *regs){
    printf("Kernel Panic!\nAn fatal error occured, and could not be recovered.\nError: ");
    printf(message);
    printf("System halting...\n");
    if(regs != 0) panic_hold(regs);
    asm volatile ("jmp .");
}

void install_irq_handler(void (*handler)(), uint8_t irqno){
    interrupt_handlers[irqno] = handler;
    // printf("called\n");
}
void *get_irq_handler(uint8_t irqno){
    return interrupt_handlers[irqno];
}
void pit_reload(uint8_t mode, uint16_t reload){
    const uint16_t PIT_CMD = 0x43;
    const uint16_t PIT_DATA = 0x40;
    outb(PIT_CMD, 0b00110000 + mode); //channel 0, low/high byte, rate generator mode
    
    outb(PIT_DATA, reload >> 8);
    outb(PIT_DATA, reload & 0xff);
    return;
}
uint32_t pit_init(uint32_t freq){
    uint32_t reload_value = 0x10000;
    if(freq <= 18){
        pit_reload(0b100, reload_value);
        uint32_t ms = (reload_value * 3000)/3579545;
        return ms;
    }
    if(freq >= 1193181){
        pit_reload(0b100, reload_value >> 16);
        uint32_t ms = (reload_value * 3000)/3579545;
        return ms;
    }
    
    reload_value = ((3579545)/(freq));
    reload_value /= 3;
    pit_reload(0b100, reload_value);
    uint32_t ms = (reload_value * 3000)/3579545;
    return ms;
}

void idt_load(){
    // for(uint32_t i = 0; i < 32; i++){
    //     set_idt_entry(i, (_isr0 + ((_isr1 - _isr0) * i)), IDT_GATE_INT, 0x10);
    //     // printf("int %d: address: %x\n", i, _isr0 + (_isr1 - _isr0) * i);
    // }
    set_idt_entry(0, _isr0, IDT_GATE_INT, 0x10);
    set_idt_entry(1, _isr1, IDT_GATE_INT, 0x10);
    set_idt_entry(2, _isr2, IDT_GATE_INT, 0x10);
    set_idt_entry(3, _isr3, IDT_GATE_INT, 0x10);
    set_idt_entry(4, _isr4, IDT_GATE_INT, 0x10);
    set_idt_entry(5, _isr5, IDT_GATE_INT, 0x10);
    set_idt_entry(6, _isr6, IDT_GATE_INT, 0x10);
    set_idt_entry(7, _isr7, IDT_GATE_INT, 0x10);
    set_idt_entry(8, _isr8, IDT_GATE_INT, 0x10);
    set_idt_entry(9, _isr9, IDT_GATE_INT, 0x10);
    set_idt_entry(10, _isr10, IDT_GATE_INT, 0x10);
    set_idt_entry(11, _isr11, IDT_GATE_INT, 0x10);
    set_idt_entry(12, _isr12, IDT_GATE_INT, 0x10);
    set_idt_entry(13, _isr13, IDT_GATE_INT, 0x10);
    set_idt_entry(14, _isr14, IDT_GATE_INT, 0x10);
    set_idt_entry(15, _isr15, IDT_GATE_INT, 0x10);
    set_idt_entry(16, _isr16, IDT_GATE_INT, 0x10);
    set_idt_entry(17, _isr17, IDT_GATE_INT, 0x10);
    set_idt_entry(18, _isr18, IDT_GATE_INT, 0x10);
    set_idt_entry(19, _isr19, IDT_GATE_INT, 0x10);
    set_idt_entry(20, _isr20, IDT_GATE_INT, 0x10);
    set_idt_entry(21, _isr21, IDT_GATE_INT, 0x10);
    set_idt_entry(22, _isr22, IDT_GATE_INT, 0x10);
    set_idt_entry(23, _isr23, IDT_GATE_INT, 0x10);
    set_idt_entry(24, _isr24, IDT_GATE_INT, 0x10);
    set_idt_entry(25, _isr25, IDT_GATE_INT, 0x10);
    set_idt_entry(26, _isr26, IDT_GATE_INT, 0x10);
    set_idt_entry(27, _isr27, IDT_GATE_INT, 0x10);
    set_idt_entry(28, _isr28, IDT_GATE_INT, 0x10);
    set_idt_entry(29, _isr29, IDT_GATE_INT, 0x10);
    set_idt_entry(30, _isr30, IDT_GATE_INT, 0x10);
    set_idt_entry(31, _isr31, IDT_GATE_INT, 0x10);

    // printf("last ")
    for(uint32_t i = 0; i < 16; i++){
        set_idt_entry(i + 32, (_irq0 + ((_irq1 - _irq0) * i)), IDT_GATE_INT, 0x10);
        // printf("int %d: address: %x\n", i + 32, _irq0 + (_irq1 - _irq0) * i);
    }
    
    idt_desc.pointer = (uint32_t)&idt_table;
    idt_desc.size = sizeof(idt_entry_t) * 256 - 1;
    for(int i = 0; i < 16; i++){
        interrupt_handlers[i] = 0;
    }
    
    uint32_t hz_ms = pit_init(50000);
    // asm volatile("lidt (%0)" : : "m"(&idt_desc));
    // outb(0x20, 0x11 );
    // outb(0xa0, 0x11);
    // outb(0x21, 0x20);
    // outb(0xa1, 0x28);
    // outb(0x21, 4);
    // outb(0xa1, 2);
    // outb(0x21, 1);
    // outb(0xa1, 1);
    // outb(0x21, 0);
    // outb(0xa1, 0);
    
    //set pic masks
    // outb(0x21, 0);
    // outb(0xa1, 0);
    load_idt(&idt_desc);
}
