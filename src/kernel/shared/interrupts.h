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

typedef struct gdt_desc{
    uint16_t size;
    uint32_t ptr;
}__attribute__((packed)) gdt_desc_t;

#define GDT_FLAGS_4KB_GRAN 8
#define GDT_FLAGS_PROT_MODE 4
#define GDT_FLAGS_LONG_MODE 2

#define GDT_SYSSEG_TSS16A 0x1
#define GDT_SYSSEG_LDT 0x2
#define GDT_SYSSEG_TSS16B 0x3
#define GDT_SYSSEG_TSS32A 0x9
#define GDT_SYSSEG_TSS32B 0xB

typedef struct gdt_entry{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_med;
    union{
        struct{
            uint8_t accessed:1;
            uint8_t rw:1;
            uint8_t dc:1;
            uint8_t executable:1;
            uint8_t type:1;
            uint8_t dpl:2;
            uint8_t present:1;
        }data_segment;
        struct{
            uint8_t sd_type:4;//System descriptor type
            uint8_t type:1;
            uint8_t dpl:2;
            uint8_t present:1;
        }system_segment;
    }access;
    uint8_t limit:4;
    uint8_t flags:4;
    uint8_t base_high;
}__attribute__((packed))gdt_entry_t;

typedef struct tss_entry{
    uint32_t last_tas;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldtr;
    uint16_t reserved;
    uint16_t iopb;
    uint32_t ssp;
}__attribute__((packed))tss_entry_t;

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

extern gdt_desc_t *get_gdt();
extern void load_tss(uint32_t segment_entry);

extern void tss_init(uint32_t entry_no);
extern void *get_tss();

extern void jump_usermode(void *esp, void *eip);

extern void reset_tss_stack();