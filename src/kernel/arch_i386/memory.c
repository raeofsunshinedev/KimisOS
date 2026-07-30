#include <stdint.h>
#include "../shared/memory.h"
#include "../shared/kstdlib.h"
#include "../shared/spinlock.h"
#include "../drivers/cpuio.h"
#define mmap_count 0x20000

uint8_t volatile pm_map[mmap_count];

uint32_t total_memory = 0;
uint32_t total_memory_unusable = 0;
uint32_t memory_limit = 0;
uint32_t volatile last_allocated = 0;
extern uint32_t _start;
extern uint32_t _end;
mmap_entry_t mmap;
uint32_t mmap_entry_c;

uint8_t *heap_map;
uint32_t heap_map_size;
void *heap_base;
uint32_t heap_first_free;

const uint32_t HEAP_ENTRY_SIZE_BITS = 4;
const uint32_t HEAP_ENTRIES_PER_INDEX = 2;

spinlock_t pm_lock;
spinlock_t page_lock;
spinlock_t heap_lock;

uint32_t pm_first_free = 0;

inline uint8_t get_heap_index(uint32_t index){
    if (index >= heap_map_size * HEAP_ENTRIES_PER_INDEX) return 0;
    return (heap_map[index / HEAP_ENTRIES_PER_INDEX] >> ((index % HEAP_ENTRIES_PER_INDEX) * HEAP_ENTRY_SIZE_BITS)) & ((1 << HEAP_ENTRY_SIZE_BITS) - 1);
}

inline void set_heap_index(uint32_t index, uint32_t flags){
    if(index >= heap_map_size * HEAP_ENTRIES_PER_INDEX) return;
    uint32_t byte = index / HEAP_ENTRIES_PER_INDEX;
    uint32_t shift = (index % HEAP_ENTRIES_PER_INDEX) * HEAP_ENTRY_SIZE_BITS;
    uint8_t mask = ((1 << HEAP_ENTRY_SIZE_BITS) - 1) << shift;
    
    heap_map[byte] =
        (heap_map[byte] & ~mask) |
        ((flags << shift) & mask);
}

inline void *heap_index_to_address(uint32_t index){
    return heap_base + index * PAGE_SIZE_BYTES;
}

inline uint32_t heap_address_to_index(void *address)
{
    printf("Addr: %x Base: %x, %x, %d\n", address, heap_base, (uint32_t)heap_base + heap_map_size * HEAP_ENTRIES_PER_INDEX * PAGE_SIZE_BYTES, (uint32_t)address < (uint32_t)heap_base);
    if ((uint32_t)address >= (uint32_t)heap_base + heap_map_size * HEAP_ENTRIES_PER_INDEX * PAGE_SIZE_BYTES || (uint32_t)address < (uint32_t)heap_base){
        return (uint32_t)-1;
    }
    return ((uint32_t)address - (uint32_t)heap_base) / PAGE_SIZE_BYTES;
}

uint32_t heap_init(uint32_t size_bytes){
    spinlock_init(&heap_lock);
    uint32_t heap_pages = (size_bytes + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES;
    heap_map_size = heap_pages/HEAP_ENTRIES_PER_INDEX;
    uint32_t heap_map_size_pages = (heap_map_size + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES;
    mlog("KERNEL", "Heap map size: %d, %d\n", MLOG_PRINT, heap_map_size, heap_map_size_pages);
    heap_map = kmalloc(heap_map_size_pages);
    // printf("Kernel heap size: %x", heap_pages);
    heap_base = kmalloc(heap_pages);
    // printf("Test\n");
    heap_first_free = 0;
    
    for(uint32_t i = 0; i < heap_map_size; i++){
        heap_map[i] = 0;
    }
    
    if(heap_base == 0){
        PANIC("Not enough memory for heap!\n");
    }
    
    mlog("KERNEL", "Initializing permanent kernel heap. Base: %x, Size: %x, Pages: %d, Limit: %x\n", MLOG_PRINT, heap_base, size_bytes, heap_pages, heap_base + size_bytes);
    // heap_map[0] = 0xa5;
    // uint8_t heap_0 = get_heap_index(0);
    // uint8_t heap_1 = get_heap_index(1);
    // printf("Test: %x, %x\n", heap_0, heap_1);
    
    // set_heap_index(0, 0xa);
    // set_heap_index(1, 0x5);
    
    // heap_0 = get_heap_index(0);
    // heap_1 = get_heap_index(1);
    // printf("Test1: %x, %x\n", heap_0, heap_1);
    // printf("Test2: %x, %x\n", heap_index_to_address(0), heap_address_to_index(heap_index_to_address(2)));
    
}

void *heap_alloc(uint32_t size_pgs){
    spinlock_acquire(&heap_lock);
    uint32_t max_index = heap_map_size * HEAP_ENTRIES_PER_INDEX;
    uint32_t start = 0;
    for(uint32_t i = heap_first_free; i < max_index; i++){
        uint32_t index_data = get_heap_index(i);
        uint8_t found = 1;
        for(uint32_t j = 0; j < size_pgs; j++){
            if(get_heap_index(i+j)){
                found = 0;
                i += j;
                break;
            }
        }
        if(found){
            start = i;
            break;
        }
    }
    if(start == 0 && get_heap_index(start)){
        return 0;
        spinlock_release(&heap_lock);
    }
    for(uint32_t i = 0; i < size_pgs; i++){
        uint32_t page_flags = KMALLOC_USED | (KMALLOC_LINK_LAST * (i > 0)) | (KMALLOC_LINK_NEXT * (i < size_pgs - 1));
        set_heap_index(start + i, page_flags);
        // printf("%d @ %d\n", page_flags, start + i);
    }
    spinlock_release(&heap_lock);
    return heap_base + start * PAGE_SIZE_BYTES;
}

void *heap_free(void* addr){
    spinlock_acquire(&heap_lock);
    uint32_t heap_index = (addr - heap_base) >> 12;
    if(heap_index < heap_map_size * HEAP_ENTRIES_PER_INDEX) return 0;
    while(get_heap_index(heap_index) & KMALLOC_LINK_LAST){
        heap_index--;
    }
    while(1){
        uint8_t flags = get_heap_index(heap_index);
        set_heap_index(heap_index, 0);
        
        if (!(flags & KMALLOC_LINK_NEXT))
        break;
        
        heap_index++;
    }
    spinlock_release(&heap_lock);
    return 0;
}

uint32_t pm_alloc(){
    // asm("cli");
    spinlock_acquire(&pm_lock);
    for(uint32_t i = pm_first_free; i < mmap_count; i++){
        for(int j = 0; j < 8; j++){
            if(!(pm_map[i] & (1 << j))){
                pm_map[i] |= (1 << j);
                if(i > pm_first_free){
                    pm_first_free = i;
                }
                spinlock_release(&pm_lock);
                return (i << 3 | j) << 12;
            }
        }
    }
    spinlock_release(&pm_lock);
    return 0;
    // asm("sti");
}
uint32_t pm_alloc_index(uint32_t index){
    // asm("cli");
    spinlock_acquire(&pm_lock);
    for(uint32_t i = 0; i < 2; i++){
        for(int j = 0; j < 8; j++){
            // if(!(pm_map[i] & (1 << j))){
            pm_map[i + index] |= (1 << j);
                // }
        }
    }
    spinlock_release(&pm_lock);
    return (index << 3) << 12;
    // asm("sti");
}
uint32_t pm_alloc_64kaligned(){
    // asm("cli");
    uint32_t cont = 0;
    uint32_t current = 0;
    spinlock_acquire(&pm_lock);
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
            spinlock_release(&pm_lock);
            return pm_alloc_index(current);
        }
        current = i;
        cont++;
    }
    spinlock_release(&pm_lock);
    return 0;
    // asm("sti");
}
void pm_free(uint32_t address){
    spinlock_acquire(&pm_lock);
    pm_map[address >> 15] &= ~(1 << ((address>>12) & 7));
    if((address >> 15) < pm_first_free){
        pm_first_free = address >> 15;
    }
    spinlock_release(&pm_lock);
}
void pm_reserve(uint32_t address){
    spinlock_acquire(&pm_lock);
    pm_map[address >> 15] |= 1 << ((address>>12) & 7);
    pm_first_free = address >> 15;
    spinlock_release(&pm_lock);
}
int pm_init(kernel_info_t *kernel_info){
    spinlock_init(&pm_lock);
    spinlock_init(&page_lock);
    pm_first_free = 0;
    total_memory_unusable = 0;
    heap_base = 0;
    mmap_entry_t *mmap = (mmap_entry_t*)(kernel_info->mmap_ptr);
    for(uint32_t i = 0; i < mmap_count; i++){
        pm_map[i] = 0xff;
    }
    mlog("KERNEL", "   BASE  | LENGTH | TYPE\n           --------|--------|----\n", MLOG_PRINT);
    for(uint32_t i = 0; i < kernel_info->mmap_entry_count; i++){
        for(uint32_t j = 0; j < (mmap[i].entry_length >> 12); j++){
            if(i != 0 && mmap[i].entry_base + (j << 12) < mmap[i-1].entry_base + mmap[i-1].entry_length){
                continue;
            }
            if(mmap[i].type != BIOS_MMAP_USABLE){
                total_memory_unusable++;
                pm_map[(mmap[i].entry_base >> 15) + (j >> 3)] |= 1 << (j & 7) + ((mmap[i].entry_base >> 12) & 7);
            }
            else{
                pm_map[(mmap[i].entry_base >> 15) + (j >> 3)] &= ~(1 << ((j & 7) + ((mmap[i].entry_base >> 12) & 7)));
            }
            // if(j % 8 == 7){
            //     printf("%d, %x, %d\n", j >> 3, pm_map[(mmap[i].entry_base >> 12 ) + (j >> 3)], mmap[i].type);
            // }
        }
        mlog("KERNEL", " %x|%x|%d\n", MLOG_PRINT, (uint32_t)mmap[i].entry_base, (uint32_t)mmap[i].entry_length, mmap[i].type);
    }
    for(uint32_t i = 0; i < 512; i++){
        total_memory_unusable++;
        pm_reserve(i * 4096);
    }
    
    // printf("%x", pm_map);
    void *kernel_addr = (void *)_start;
    while(get_paddr(kernel_addr)){
    
        pm_reserve(get_paddr(kernel_addr));
        kernel_addr += 0x1000;
    }
}
void map(void *vaddr, void *paddr, uint32_t flags){
    // asm("cli");
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
    // asm("sti");
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
    return (void *)(paddr+i*4096);
}
void *kmalloc(uint32_t size_pgs){
    // asm("cli");
    if(heap_base != 0){
        return heap_alloc(size_pgs);
    }
    spinlock_acquire(&page_lock);
    uint32_t i = 0xc0000000 >> 12; //4mb/4096 (start search at 1mb line)
    while(i < (1 << 22)){
        uint8_t found = 1;
        for(uint32_t j = 0; j < size_pgs; j++){
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
            if(physaddr == 0){
                kfree((void *)(i<<22));
                spinlock_release(&page_lock);
                return 0;
            }
            map((void *)((i + j) << 12), (void*)physaddr, flags);
        }
        // asm("sti");
        spinlock_release(&page_lock);
        return (void*)(i << 12);
    }
    // asm("sti");
    spinlock_release(&page_lock);
    return 0;
}
void *kmalloc_page_paddr(uint32_t paddr, uint32_t size_pgs){
    uint32_t i = 0xc0000000 >> 12; //4mb/4096 (start search at 1mb line)
    spinlock_acquire(&page_lock);
    while(i < (1 << 22)){
        uint8_t found = 1;
        for(uint32_t j = 0; j < size_pgs; j++){
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

            map((void *)((i + j) << 12), (void*)physaddr, flags);
        }
        spinlock_release(&page_lock);
        return (void*)(i << 12);
    }
    spinlock_release(&page_lock);
    return 0;
}
void *kfree(void *vaddr){
    if(heap_base != 0){
        heap_free(vaddr);
    }
    if(!get_pflags(vaddr)){
        return 0;
    }
    spinlock_acquire(&page_lock);
    uint32_t addr = (uint32_t)vaddr;
    addr &= ~0xfff;
    while(get_pflags((void *)addr)&PT_LINK_L) addr -= 0x1000;
    while (1) {
        uint32_t flags = get_pflags((void *)addr);
        uint32_t paddr = get_paddr((void *)addr);

        unmap((void *)addr);
        pm_free(paddr);

        if (!(flags & PT_LINK_N))
            break;

        addr += 0x1000;
    }
    spinlock_release(&page_lock);
    return 0;
}