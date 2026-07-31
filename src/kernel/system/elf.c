#include "elf.h"
#include "../shared/kstdlib.h"
#include "../shared/string.h"
#include "../shared/memory.h"

#define MODULE_NAME "ELFLOAD"

char *type[] = {"", "reloc", "exec", "shared", "core"};
char *flags[] = {"---", "X--", "-W-", "XW-", "--R", "X-R", "-WR", "XWR"};

void *load_segment(program_entry_t entry, void *file_data, void *base_segment, uint32_t map_flags){
    // mlog("ELFLOAD", "p_addr: %d\n", MLOG_DEBUG, entry.paddr);
    printf("Type: %x, Flags: %s, Vaddr: %x, Offset: %x, Size: %x, Align: %x\n", entry.type, flags[entry.flags], entry.paddr, entry.data_offset, entry.msize, entry.alignment);
    uint8_t *segment;
    // printf("Base: %x\n", base_segment);
    if(entry.type == 1){
        if(((elf_header_t *)(file_data))->type == ELF_TYPE_SHARED){
            segment = (base_segment + (entry.vaddr));
            memclr(segment, entry.msize);
            
            for(uint32_t i = 0; i < entry.fsize; i++){
                segment[i] = ((uint8_t*)file_data)[i + entry.data_offset];
            }
        }
        else if(((elf_header_t *)(file_data))->type == ELF_TYPE_EXE){
            //This is where Not shared objects and drivers will be loaded
            // I'll do this when I stop hyperfixating on the other parts of my operating system
        }
    }
    if (entry.type == 2){
        //What is this for?
        //TODO: Find out how to handle Dynamic sections
    }
    
    // printf("data_offset: %x\n", data_offset);
    return base_segment;
}

//returns entry address of elf file
void *load_elf(void *file_data, uint32_t map_flags){
    elf_header_t *header = file_data;
    if(header->magic != 0x464c457f || header->type == ELF_TYPE_CORE){
        return 0;
    }
    program_entry_t *program_header = file_data + header->program_header_offset;
    uint32_t program_header_count = header->program_entry_count;
    mlog(MODULE_NAME, "PROGRAM ENTRIES: %d\n", MLOG_PRINT, program_header_count);
    uint32_t size_pages = 0;
    uint32_t limit = 0;
    for(uint32_t i = 0; i < program_header_count; i++){
        // size_pages += (program_header[i].msize + PAGE_SIZE_BYTES - 1) / PAGE_SIZE_BYTES;
        limit = unsigned_max(limit, program_header[i].vaddr + program_header[i].msize);
    }
    
    printf("Limit: %x, Pages: %d", limit, (limit + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES);
    size_pages = (limit + PAGE_SIZE_BYTES - 1)/PAGE_SIZE_BYTES;
    
    void *base_segment = kmalloc(size_pages);
    for(uint32_t i = 0; i < program_header_count; i++){
        base_segment = load_segment(program_header[i], file_data, base_segment, map_flags);
    }
    return base_segment + header->entry_offset;
}