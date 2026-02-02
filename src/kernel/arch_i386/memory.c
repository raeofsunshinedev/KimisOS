#include <stdint.h>
#include "../shared/memory.h"
#include "../shared/kstdlib.h"
#include "../drivers/cpuio.h"
#define mmap_count 0x20000

uint8_t volatile pm_map[mmap_count];

uint32_t total_memory = 0;
uint32_t total_memory_usable = 0;
uint32_t volatile last_allocated = 0;
extern uint32_t _start;
extern uint32_t _end;
mmap_entry_t mmap;
uint32_t mmap_entry_c;

uint32_t pm_alloc(){
    for(uint32_t i = 0; i < mmap_count; i++){
        for(int j = 0; j < 8; j++){
            if(!(pm_map[i] & (1 << j))){
                pm_map[i] |= (1 << j);
                return (i << 3 | j) << 12;
            }
        }
    }
}
uint32_t pm_alloc_index(uint32_t index){
    for(uint32_t i = 0; i < 2; i++){
        for(int j = 0; j < 8; j++){
            // if(!(pm_map[i] & (1 << j))){
            pm_map[i + index] |= (1 << j);
                // }
        }
    }
    return (index << 3) << 12;
}
uint32_t pm_alloc_64kaligned(){
    uint32_t cont = 0;
    uint32_t current = 0;
    for(uint32_t i = 0; i < mmap_count; i++){
        if(i & 1 && cont < 1){
            continue;
        }
        for(int j = 0; j < 8; j++){
            if(!(pm_map[i] & (1 << j))){
                continue;
            }else{
                // j++;
                cont = -1;
                break;
            }
        }
        if(cont == 1){
            return pm_alloc_index(current);
        }
        current = i;
        cont++;
    }
}
void pm_free(uint32_t address){
    pm_map[address >> 15] &= ~(1 << ((address>>12) & 7));
}
void pm_reserve(uint32_t address){
    pm_map[address >> 15] |= 1 << ((address>>12) & 7);
}
int pm_init(kernel_info_t *kernel_info){
    mmap_entry_t *mmap = (mmap_entry_t*)(kernel_info->mmap_ptr);
    // printf("mmap count: %d\n| start  | length |type|\n|--------|--------|----|\n", kernel_info->mmap_entry_count);
    for(uint32_t i = 0; i < mmap_count; i++){
        pm_map[i] = 0xff;
    }
    // mlog("KERNEL", "   BASE  | LENGTH | TYPE\n           --------|--------|----\n", MLOG_PRINT);
    for(uint32_t i = 0; i < kernel_info->mmap_entry_count; i++){
        for(uint32_t j = 0; j < (mmap[i].entry_length >> 12); j++){
            if(mmap[i].entry_base + (j << 12) <= mmap[i-1].entry_base + mmap[i-1].entry_length){
                continue;
            }
            // printf("j = %d\nIndex:%x\n", j, (mmap[i].entry_base >> 12) + (j & 3));
            if(mmap[i].type != BIOS_MMAP_USABLE){
                pm_map[(mmap[i].entry_base >> 15) + (j >> 3)] |= 1 << (j & 7) + ((mmap[i].entry_base >> 12) & 7);
            }
            else{
                // uint8_t tmp = pm_map[(mmap[i].entry_base >> 12 ) + (j >> 3)];
                // tmp &= ~(1 << (j & 3));
                pm_map[(mmap[i].entry_base >> 15) + (j >> 3)] &= ~(1 << ((j & 7) + ((mmap[i].entry_base >> 12) & 7)));
            }
            // if(j % 8 == 7){
            //     printf("%d, %x, %d\n", j >> 3, pm_map[(mmap[i].entry_base >> 12 ) + (j >> 3)], mmap[i].type);
            // }
        }
        // mlog("KERNEL", " %x|%x|%d\n", MLOG_PRINT, (uint32_t)mmap[i].entry_base, (uint32_t)mmap[i].entry_length, mmap[i].type);
        // printf("%x",  mmap[i].entry_length);
        // printf("|%d   |\n", mmap[i].type);
    }
    for(uint32_t i = 0; i < 512; i++){
        pm_reserve(i * 4096);
    }
    
    // printf("%x", pm_map);
    void *kernel_addr = (void *)_start;
    while(get_paddr(kernel_addr)){
    
        // printf("%x\n", kernel_addr);
        pm_reserve(get_paddr(kernel_addr));
        kernel_addr += 0x1000;
    }
}
void map(void *vaddr, void *paddr, uint32_t flags){
    uint32_t pd_index = (uint32_t)vaddr >> 22;
    uint32_t pt_index = (uint32_t)vaddr >> 12 & 0x3ff;
    
    uint32_t *pd = (uint32_t*)0xfffff000;
    if(!pd[pd_index]){
        pd[pd_index] = pm_alloc() | 1;
        // printf("allocating %x\n", pd[pd_index]);
        asm volatile("invlpg (%0)" : : "b"(0xffc00000 + (pd_index * 0x400)) : "memory");
        uint32_t *pt = (uint32_t *)(0xffc00000 + (pd_index * 0x1000));
        for(uint32_t i = 0; i < 0x400; i++)pt[i] = 0;
    }
    uint32_t *pt = (uint32_t *)(0xffc00000 + (pd_index * 0x1000));
    // printf("%x %x\n", vaddr, paddr);
    pt[pt_index] = (uint32_t)paddr | flags;
    asm volatile("invlpg (%0)  " : : "b"(vaddr) : "memory");
}
void unmap(void *vaddr){
    uint32_t pd_index = (uint32_t)vaddr >> 22;
    uint32_t pt_index = (uint32_t)vaddr >> 12 & 0x3ff;
    // printf("unmap called");
    uint32_t *pd = (uint32_t *)0xfffff000;
    if(!pd[pd_index]){
        return;
    }
    uint32_t *pt = (uint32_t *)(0xffc00000 + (pd_index * 0x1000));
    pt[pt_index] = 0;
    // uint32_t paddr = pt[pt_index];
    // paddr ^= (paddr & 0xfff);
    // pm_free(paddr);
    asm volatile("invlpg (%0)  " : : "b"(vaddr) : "memory");
    return;
}
void map_4mb(void *vaddr, void *paddr, uint32_t flags){
    //just wanted to get the prototype out
    //!TODO: Finish mapping and unmapping 4mb pages
}
uint32_t get_paddr(void *vaddr){
    //!TODO: Support 4mb pages
    
    uint32_t pd_index = (uint32_t)vaddr >> 22;
    uint32_t pt_index = (uint32_t)vaddr >> 12 & 0x3ff;
    // printf("unmap called");
    uint32_t *pd = (uint32_t *)0xfffff000;
    if(!pd[pd_index]){
        return 0;
    }
    uint32_t *pt = (uint32_t *)(0xffc00000 + (pd_index * 0x1000));
    return pt[pt_index] & 0xfffff000;
}
uint32_t get_pflags(void *vaddr){
    uint32_t pd_index = (uint32_t)vaddr >> 22;
    uint32_t pt_index = (uint32_t)vaddr >> 12 & 0x3ff;
    uint32_t *pd = (uint32_t *)0xfffff000;
    if(!pd[pd_index]){
        return 0;
    }
    uint32_t *pt = (uint32_t *)(0xffc00000 + (0x1000 * pd_index));
    return (pt[pt_index] & 0xfff);
}
void *get_new_page(uint32_t flags){
    uint32_t i = 0;
    uint32_t paddr = pm_alloc();
    while(get_paddr((void*)paddr + (i * 4096))){
        i++;
    }
    map((void *)paddr+(i*4096), (void *)paddr, flags);
}
void *kmalloc(uint32_t size_pgs){
    // static uint8_t fake = 0;
    uint32_t i = 0xc0000000 >> 12; //4mb/4096 (start search at 1mb line)
    while(i < (1 << 22)){
        uint8_t found = 1;
        for(uint32_t j = 0; j < size_pgs; j++){
            // printf("found at %x, %x\n", (i + j) << 12, get_paddr((void*)((i + j) << 12)));
            if(get_pflags((void *)((i + j) << 12))){
                found = 0;
                break;
            }
        }
        if(!found){
            i++;
            continue;
        }
        for(uint32_t j = 0; j < size_pgs; j++){
            uint32_t flags = PT_PRESENT | PT_SYS | (PT_LINK_L * (j != 0)) | (PT_LINK_N * (j < (size_pgs - 1)) | PT_PCD);
            uint32_t physaddr = pm_alloc();
            // if(j == 0) printf("%x", physaddr);
            // printf("Mapping %x to %x\n", (i + j) << 12, physaddr);
            // if(fake) return 0;
            map((void *)((i + j) << 12), (void*)physaddr, flags);
        }
        // fake = !fake;
        // for(;;);
        return (void*)(i << 12);
    }
    return 0;
}
void *kmalloc_page_paddr(uint32_t paddr, uint32_t size_pgs){
    uint32_t i = 0xc0000000 >> 12; //4mb/4096 (start search at 1mb line)
    while(i < (1 << 22)){
        uint8_t found = 1;
        for(uint32_t j = 0; j < size_pgs; j++){
            // printf("found at %x, %x\n", (i + j) << 12, get_paddr((void*)((i + j) << 12)));
            if(get_pflags((void *)((i + j) << 12))){
                found = 0;
                break;
            }
        }
        if(!found){
            i++;
            continue;
        }
        for(uint32_t j = 0; j < size_pgs; j++){
            uint32_t flags = PT_PRESENT | PT_SYS | (PT_LINK_L * (j != 0)) | (PT_LINK_N * (j < (size_pgs - 1)) | PT_PCD);
            uint32_t physaddr = paddr + j * 4096;
            // if(j == 0) printf("%x", physaddr);
            // printf("Mapping %x to %x\n", (i + j) << 12, physaddr);
            // if(fake) return 0;
            map((void *)((i + j) << 12), (void*)physaddr, flags);
        }
        // fake = !fake;
        // for(;;);
        return (void*)(i << 12);
    }
}
void *kfree(void *vaddr){
    if(!get_pflags(vaddr)){
        return 0;
    }
    uint32_t addr = (uint32_t)vaddr;
    addr &= ~0xfff;
    while(get_pflags((void *)addr)&PT_LINK_L) addr -= 0x1000;
    do{
        uint32_t paddr = get_paddr((void *)addr);
        pm_free(paddr);
        unmap((void *)addr);
        addr += 0x1000;
    }while(get_pflags((void *)addr)&PT_LINK_N);
    return 0;
}