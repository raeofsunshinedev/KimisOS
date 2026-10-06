#pragma once
#include <stdint.h>

#define ELF_SIG 0x464c457f
#define ELF_BITS_32 1
#define ELF_BITS_64 2
#define ELF_ENDIAN_LITTLE 1
#define ELF_ENDIAN_BIG 2
#define ELF_TYPE_RELOC 1
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_SHARED 3
#define ELF_TYPE_CORE 4


typedef struct{
    uint32_t magic;
    uint8_t bits;
    uint8_t endianness;
    uint8_t elf_head_version;
    uint8_t abi;
    uint64_t padding;
    uint16_t type;
    uint16_t isa;
    uint32_t elf_version;
    uint32_t entry_offset;
    uint32_t program_header_offset;
    uint32_t section_table_offset;
    uint32_t flags;
    uint16_t elf_header_size;
    uint16_t program_entry_size;
    uint16_t program_entry_count;
    uint16_t section_entry_size;
    uint16_t section_entry_count;
    uint16_t section_strtab_index;
}__attribute__((packed)) elf_header_t;

typedef struct{
    uint32_t type;
    uint32_t data_offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t fsize;//size in file
    uint32_t msize;//size in memory
    uint32_t flags;
    uint32_t alignment;
}__attribute__((packed))program_entry_t;

#define PROG_ENT_FLAGS_EXE 1
#define PROG_ENT_FLAGS_WRITE 2
#define PROG_END_FLAGS_READ 4

#define ELF_TYPE_RELOC 1
#define ELF_TYPE_EXE 2
#define ELF_TYPE_SHARED 3
#define ELF_TYPE_CORE 4

void *load_elf_dynamic(void *file_data, uint32_t map_flags);