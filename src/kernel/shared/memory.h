#pragma once
#include <stdint.h>
#include "kstdlib.h"

#define PD_PRESENT  0x0001
#define PD_USER_RW  0x0002
#define PD_USER     0x0004
#define PD_PWT      0x0008
#define PD_PCD      0x0010
#define PD_ACCESSED 0x0020
#define PD_DIRTY    0x0040
#define PD_PAGE_SZ  0x0080
#define PD_GLOBAL   0x0100
#define PD_PAT      0x1000
#define PD_LINK_N   0x0200
#define PD_LINK_L   0x0400
#define PD_SYS      0x0800

#define PT_PRESENT  0X0001
#define PT_USER_RW  0x0002
#define PT_USER     0x0004
#define PT_PWT      0x0008
#define PT_PCD      0x0010
#define PT_ACCESSED 0x0020
#define PT_DIRTY    0x0040
#define PT_PAT      0x0080
#define PT_GLOBAL   0x0100
#define PT_LINK_N   0x0200
#define PT_LINK_L   0x0400
#define PT_SYS      0x0800

#define BIOS_MMAP_USABLE 1
#define BIOS_MMAP_RESERVED 2
#define BIOS_MMAP_RECLAIM 3
#define BIOS_MMAP_NVS 4
#define BIOS_MMAP_BAD_MEMORY 5

#define KMALLOC_USED        1
#define KMALLOC_LINK_NEXT   2
#define KMALLOC_LINK_LAST   4

typedef struct BIOS_data{
    uint16_t com_ports[4];
    uint16_t lpt_ports[3];
    uint16_t ebda_addr;
    uint16_t hw_bitflags;
    uint16_t kb_to_ebda;
    uint16_t kbd_state;
    char     kbd_buffer[32];
    uint8_t display_mode;
    uint16_t columns_text;
    uint16_t base_video_port;
    uint16_t irq0_boot;
    uint8_t hard_drives;
    uint16_t kbd_buffer_start;
    uint16_t kbd_buffer_end;
    uint8_t kbd_led_state;
}__attribute__((packed)) bda_t;

typedef struct memory_map_entry{
    uint64_t entry_base;
    uint64_t entry_length;
    uint32_t type;
    uint32_t extended_bitflags;
}__attribute__((packed))mmap_entry_t;

void map(void *vaddr, void *paddr, uint32_t flags);
void unmap(void *vaddr);
void map_4mb(void *vaddr, void* paddr, uint32_t flags);
uint32_t get_paddr(void *addr);
uint32_t pm_alloc();
void pm_reserve(uint32_t addr);
void pm_free(uint32_t addr);
uint32_t pm_alloc_64kaligned();
void *kmalloc_page_paddr(uint32_t paddr, uint32_t size);

inline void set_cr3(void *cr3){
    asm volatile("mov %0, %%eax; mov %%eax, %%cr3":: "r"(cr3));
}
inline void *get_cr3(){
    void *cr3;
    asm volatile("mov %%cr3, %%eax;mov %0, %%eax": "=r"(cr3));
    return cr3;
}
int pm_init(kernel_info_t *kernel_info);

void *kmalloc(uint32_t size_pgs);//allocates in multiples of 4096
void *kfree(void *vaddr);
void *kmalloc_page_paddr(uint32_t paddr, uint32_t size_pgs);