;   Kimi's Bootloader - Custom Booting solution for Kimi's OS
;   (c) 2024 Notsomeidiot123 on Github
;   Documentation available at https://github.com/notsomeidiot123/KimisOS


org 0x7c00

bits 16

%macro debug 0
    mov ah, 0xe
    mov al, 0x41
    int 0x10
    jmp $
%endmacro

;TODO:
;Enable A20 line                            |x|
;Enable unreal mode                         |x|
;Load GDT                                   |x|
;Load part2                                 |x|
;Get video mode information                 |0|
;Set video mode                             |0|
;get memory map                             |x|
;enable paging                              |x|
;read filesystem                            |x|
;load kernel                                |x|
;parse ELF format                           |x|
;pass memory and video information to kernel|x|
;relocate and jump to kernel                |x|

;TODO 7/4/26
;Remember to ZERO THE BSS

jump:
    jmp short start
    times 3-($-$$) db 0x90
fat_bpb:
    .oem:
        db "KIMI'SOS"
    .bytes_per_sector:
        dw 512
    .sectors_per_cluster:
        db 8
    .reserved_sectors:
        dw 32
    .fat_count:
        db 2
    .root_dir_entries:
        dw 0
    .sectors_in_volume:
        dw 0
    .media_type:
        db 0xf8
    .sectors_per_fat:
        dw 0
    .sectors_per_track:
        dw 63
    .heads:
        dw 8
    .part_start:
        dd 0
    .large_sector_count:
        dd 1048576
fat_ebpb32:
    .sectors_per_fat:
        dd 2048
    .flags:
        dw 0
    .version:
        db 0, 1
    .root_dir_cluster:
        dd 2
    .fs_info_sector:
        dw 1
    .backup_boot:
        dw 0
    .reserved:
        times 12 db 0
    .drive_num:
        db 0x80
    .ntflags:
        db 0
    .sig:
        db 0x29
    .vid_Serial:
        dd 0
    .label:
        db " Kimi's OS "
    .sysid:
        db "FAT32   "
    
start:
    mov sp, 0x7000
    mov bp, 0x7000
    mov ax, ds
    xor bx, bx
    mov ds, bx
    mov [data.part_seg], ax
    mov [data.part_off], si
    mov [data.boot_disc], dl
    jmp 0:set_cs
set_cs:
    xor cx, cx
check_a20:
    inc cx
    mov ax, 0xffff
    mov es, ax
    mov ax, [es:0x7e0e]
    cmp ax, [ds:0x7dfe]
    jne load_gdt
enable_a20:
    cmp cx, 4
    je no_a20
    cmp cx, 3
    je .fast_a20
    cmp cx, 2
    je .kb_en
    .bios_en:
        mov ax, 0x2401
        int 0x15
        jmp check_a20
    .fast_a20:
        in al, 0x92
        or al, 2
        out 0x92, al
        mov ax, 10000
        .lp:
            dec ax
            cmp ax, 0
            jge .lp
        jmp check_a20
    .kb_en:
        cli
        mov al, 0xad
        out 0x64, al

        call .wait
        mov al, 0xd0
        out 0x64, al

        call .wait2
        in al, 0x60
        push eax

        call .wait
        mov al, 0xd1
        out 0x64, al

        call .wait
        pop eax
        or al, 2
        out 0x60, al

        call .wait
        mov al, 0xae
        out 0x64, al

        call .wait
        sti
        jmp check_a20

    .wait:
        in al, 0x64
        test al,2 
        jnz .wait
        ret
    .wait2:
        in al, 0x64
        test al, 1
        jnz .wait2
        ret
load_gdt:
    lgdt [GDT_desc]
    push ds
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x8:.unreal
    .unreal:
        mov bx, 0x18
        mov ds, bx
        and al, 0xfe
        mov cr0, eax
        jmp 0x0:.real
    .real:;no actual code here, just label for readability and organization
    pop ds
load_part2:
    mov ah, 0x42
    xor edx, edx
    mov dx, word [data.part_seg]
    shl edx, 4
    xor ecx, ecx
    mov cx, word [data.part_off]
    add edx, ecx
    
    mov al, byte [edx]
    and al, 0x80 ;Rae if you are looking to see if this bootloader is compatible with an MBR, this line means it is. Go back to whatever you were working on.
    jz .read
    mov ecx, [edx + 0x8]
    
    add [DAP.sector_start], ecx
    .read:
        mov dl, byte [data.boot_disc]
        push 0
        pop ds
        mov si, DAP
        
        int 0x13
        jc load_fail
        jmp part_2
    
    jmp $
no_a20:
    mov ah, 0xe
    mov al, 'E'
    int 0x10
    mov al, '1'
    int 0x10
    jmp $
load_fail:
    mov ah, 0xe
    mov al, 'E'
    int 0x10
    mov al, '2'
    int 0x10
    jmp $
DAP:
    .sz: db 0x10
    .unused: db 0
    .read_count: dw 7
    .offset: dw 0x7e00
    .segment: dw 0
    .sector_start:
        dd 1
        dd 0
GDT_desc:
    .size: dw GD_TABLE-GD_END - 1
    .offset: dd GD_TABLE
GD_TABLE:
    .null:
        dd 0
        dd 0
    .kcode16:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0b10011010
        db 0b00001111
        db 0x00
    .kcode32:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0b10011010
        db 0b11001111
        db 0x00
    .kdata32:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0b10010010
        db 0b11001111
        db 0x00
    .ucode32:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0b11111010
        db 0b11001111
        db 0x00
    .udata32:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0b11110010
        db 0b11001111
        db 0x00
    .tss:
        dd 0
        dd 0
    .phony:
        dw 0xaa55
        dd 0
        dw 0x55aa
GD_END:
data:
    .boot_disc: db 0
    .part_seg: dw 0
    .part_off: dw 0

times 510-($-$$) db 0
db 0x55, 0xaa
fs_info:
    .id: dd 0x41615252 
    .reserved0: times 480 db 0
    .sig: dd 0x61417272
    .last_known_free: dd 0xFFFFFFFF
    .free_search_start: dd 3
    .reserved: times 12 db 0
    .trail: dd 0xAA550000 
times 1024-($-$$) db 0
part_2:
    call get_mmap
    call paging_en
    call set_video_mode
    mov esi, 0
    call cache_file_table
    ; debug
    mov edi, kernel_file
    call open_file
    ; debug
    cmp eax, -1
    je file_not_found
    add edx, 0x40000;is 1mb enough for a filesystem diver and a disk driver?
    call find_kpaddr
    ; debug
    mov esi, eax
    mov edi, [kload_paddr]
    ; debug
    call read_file
    ; debug
    jc load_fail
    ; debug
    mov esi, [kload_paddr]
    ; jmp $
    call load_elf
    ; debug
    push eax
    
    mov edi, initrd_file
    call open_file
    cmp eax, -1
    je .start32_no_initrc
    
    
    ; mov edi, disk_module_file
    ; call open_file
    ; ; debug
    ; cmp eax, -1
    ; je .find_fs_driver_no_disk
    mov [initrd.size], edx
    mov esi, eax
    and edx, 0xfffff000
    add edx, 0x2000
    add [kload_paddr], edx
    ; ; mov eax, [kload_paddr]
    ; ; jmp $
    ; ; debug
    mov edi, [kload_paddr]
    mov [initrd.ptr], edi
    push edx
    call read_file
    pop edx
    add [kload_paddr], edx
    jc load_fail
    jmp .start32
    ; ; mov esi, [kload_paddr]
    ; ; call load_elf
    ; ; debug
    ; jc load_fail
    ; jmp .find_fs_driver
    ; .find_fs_driver_no_disk:
    ;     mov ax, 0xe44
    ;     int 0x10
    ; .find_fs_driver:
    ;     mov edi, fs_module_file
    ;     call open_file
    ;     ; debug
    ;     cmp eax, -1
    ;     je .start32_no_fs
    ;     mov [fs_module_struct.size], edx
    ;     mov esi, eax
    ;     and edx, 0xfffff000
    ;     add edx, 0x2000
    ;     add [kload_paddr], edx
    ;     ; mov eax, [kload_paddr]
    ;     ; jmp $
    ;     ; debug
    ;     mov edi, [kload_paddr]
    ;     mov [fs_module_struct.ptr], edi
    ;     push edx
    ;     call read_file
    ;     pop edx
    ;     add [kload_paddr], edx
    ;     jc load_fail
    ;     jmp .find_initrc
    ;     ;
    ; .start32_no_fs:
    ;     mov ax, 0xe46
    ;     int 0x10
    ; .find_initrc:
    ;     mov edi, initrc_file
    ;     call open_file
    ;     ; debug
    ;     cmp eax, -1
    ;     je .start32_no_initrc
    ;     mov [initrc_struct.size], edx
    ;     mov esi, eax
    ;     and edx, 0xfffff000
    ;     add edx, 0x2000
    ;     add [kload_paddr], edx
    ;     ; mov eax, [kload_paddr]
    ;     ; jmp $
    ;     ; debug
    ;     mov edi, [kload_paddr]
    ;     mov [initrc_struct.ptr], edi
    ;     call read_file
    ;     jc load_fail
    ;     jmp .start32
    .start32_no_initrc:
        mov ax, 0xe49
        int 0x10
    .start32:
    ; debug
    ; push eax
    cli
    mov eax, page_table
    mov cr3, eax
    mov eax, cr4
    or eax, 1 << 4
    mov cr4, eax
    mov eax, cr0
    or eax, 0x8000_0001
    mov cr0, eax
    jmp 0x10:protected_mode
    
protected_mode:
        bits 32
        mov bx, 0x18
        mov ds, bx
        mov es, bx
        mov ss, bx
        mov gs, bx
        mov fs, bx
        pop eax
        mov esi, k_info
        ; jmp $
        jmp eax
    jmp $
bits 16
file_not_found:
    mov ah, 0xe
    mov al, 'E'
    int 0x10
    mov al, '5'
    int 0x10
load_kernel:
get_mmap:
    pushad
    push ds
    push es
    
    push 0x200
    pop es
    mov di, 0
    
    mov edx, 0x534D4150
    xor ebx, ebx
    xor esi, esi
    .get_map:
        mov eax, 0xe820
        mov ecx, 24
        int 0x15
        jc .ret
        mov eax, [es:di + 8]
        or eax, [es:di + 16]
        jz .get_map
        inc si
        or dword [es:di + 20], 1
        add di, 24
        cmp ebx, 0
        je .ret
        jmp .get_map
    
    .ret:
        cmp si, 0
        je .ret_err
        mov [es:di], dword 0
        mov [k_info.memory_map_count], si
        pop es
        pop ds
        popad
        mov ax, 0
        ret
    .ret_err:
        pop es
        pop ds
        popad
        mov ax, 1
        ret
set_video_mode:
    mov ah, 0x0
    mov al, 0x3
    int 0x10
    ret
paging_en:
    mov eax, cr4
    or eax, 0x00000010 ;enable 4mb pages
    mov cr4, eax
    
    mov eax, page_table;page table base
    mov [eax], dword 0
    or [eax], dword 0b0_0001_1000_1001;present, Write-through, page size 4mb, global, present
    mov cr3, eax
    
    ;enable recursive paging
    mov [eax + 1023*4], eax
    or [eax + 1023*4], dword 0b0_0000_0000_0011
    
    ret
open_file:
    ;only looks in the root directory
    ;edi = file_to_search
    ;esi = ptr to load file [TODO: IMPLEMENT]
    ;eax = return value (first cluster)
    ;edx = return value (file size)
    push edi
    xor esi, esi
    xor ecx, ecx
    xor edx, edx
    mov esi, [fat_bpb.part_start];
    add si, [fat_bpb.reserved_sectors]
    mov cl, [fat_bpb.fat_count]
    mov dx, [fat_ebpb32.sectors_per_fat]
    
    mov ax, dx
    xor dx, dx
    mul cx
    add si, ax
    xor cx, cx
    mov ax, [fat_ebpb32.root_dir_cluster]
    sub ax, 2
    mov cl, [fat_bpb.sectors_per_cluster]
    mul cx
    add si, ax
    ; jmp $
    mov byte [DAP.segment], 0
    mov [DAP.sector_start], esi
    mov word [DAP.read_count], 8
    mov word [DAP.offset], root_dir
    
    mov si, DAP
    mov ah, 0x42
    mov dl, [data.boot_disc]
    ; jmp $
    int 0x13
    jc .not_found_ret
    xor ecx, ecx
    lea esi, [root_dir]
    push word 0
    pop es
    .chklp:
        ; add esi, 0x20
        mov edi, [esp]
        cmp byte [esi], 0
        mov ax, [esi]
        je .not_found_ret
        mov ecx, 11
        push esi
        repe cmpsb
        pop esi
        ; jmp $
        je .found
        add esi, 0x20
        jmp .chklp
    .found:
        pop edi
        ;eax = first cluster
        ;edx = file size
        mov ax, [esi + 20]
        shl eax, 16
        mov ax, [esi + 26]
        mov edx, [esi + 28]
        ; jmp $
        ret
    
    .not_found_ret:
        pop edi
        mov eax, -1
        ret
    jmp $
    ; debug
read_file:
    ;args: esi = first cluster in chain
    ;edi = pointer to data
    push esi
    sub esi, 2
    xor ecx, ecx
    xor edx, edx
    mov eax, esi
    mov cl, [fat_bpb.sectors_per_cluster]
    mul ecx; eax  = offset from file table
    ; jmp $

    push eax
    xor eax, eax
    xor ecx, ecx
    
    mov eax, [fat_ebpb32.sectors_per_fat]
    xor edx, edx
    mov cl, [fat_bpb.fat_count]
    mul ecx
    
    xor edx, edx
    mov dx, [fat_bpb.reserved_sectors]
    pop ecx
    add eax, ecx
    add eax, edx
    mov [DAP.sector_start], eax
    mov ecx, [fat_bpb.part_start]
    add [DAP.sector_start], ecx
    mov edx, [DAP.sector_start]
    ; cmp esi, 3
    ; je $
    xor ecx, ecx
    mov cl, [fat_bpb.sectors_per_cluster]
    mov [DAP.read_count], cx
    ; push edi
    ; shr edi, 16
    ; mov [DAP.segment], di
    ; pop edi
    ; mov [DAP.offset], di
    ; jmp $
    mov word [DAP.segment], 0
    mov word [DAP.offset], file_buffer
    
    mov ah, 0x42
    mov dl, [data.boot_disc]
    mov si, DAP
    int 0x13
    jc .exit_err
    
    
    ; debug
    xor eax, eax
    mov al, [fat_bpb.sectors_per_cluster]
    xor edx, edx
    xor ecx, ecx
    mov cx, [fat_bpb.bytes_per_sector]
    mul ecx
    ; push eax
    
    mov esi, file_buffer
    mov ebx, eax
    call mmove
    ; debug 
    pop esi
    call read_fat
    ; jmp $
    ; debug
    cmp eax, 0xffffffff
    je .exit_success
    
    ; jmp $
    mov esi, eax
    add edi, ebx
    jmp read_file
    
    .exit_err:
        mov eax, 1
        stc
        ret
    .exit_success:
        mov eax, 0
        clc
        ; debug
        ret
    jmp $
    
read_fat:
    ;args:
    ;esi: index
    ;return reg: eax
    push esi
    shr esi, 12
    cmp esi, [loaded_fat_block]
    mov eax, [loaded_fat_block]
    
    je .read
    
    call cache_file_table
    ; mov eax, [loaded_fat_block]
    ; mov esi, [file_table + 0x4 * 3]
    
    .read:
        pop esi
        and esi, 0xfff
        
        mov eax, [esi * 0x4 + file_table]
        
        ret
cache_file_table:
    ;args: 
    ;esi: fat block index (in 4kb)
    ;reads all data from file
    mov eax, 4096
    push edx
    xor edx, edx
    push ecx
    xor ecx, ecx
    mov cx, [fat_bpb.bytes_per_sector]
    div ecx
    
    push eax ;will be important for the DAP
    mov [loaded_fat_block], esi
    
    mov ecx, eax
    
    mov eax, esi
    xor edx, edx
    mul ecx;eax = start sector
    ;sector_to_read = start_sector + fat_bpb.reserved_sectors
    xor edx, edx
    mov dx, [fat_bpb.reserved_sectors]
    add eax, edx;sector_to_read
    mov [DAP.sector_start], eax
    pop eax
    mov [DAP.read_count], ax
    mov word [DAP.offset], file_table
    mov ah, 0x42
    mov si, DAP
    mov dl, [data.boot_disc]
    int 0x13
    jc load_fail
    
    pop ecx
    pop edx
    ret
    jmp $

mmove:
    ;esi = src
    ;edi = dest
    ;ebx = count
    ; jmp $
    push eax
    push ecx
    
    ; debug
    xor ecx, ecx
    .mlp:
    ; jmp $
        cmp ecx, ebx
        jge .ret
        mov al, [ds:esi + ecx]
        mov [ds:edi + ecx], al
        inc ecx
        jmp .mlp
    .ret:
    ; debug
        pop ecx
        pop eax
        ; jmp $
        ret
find_kpaddr:
    ;edx = size of file in bytes
    push edx
    push esi
    push eax
    mov esi, [k_info.memory_map_ptr]
    mov ecx, 24
    .sloop:
        mov eax, [esi + ecx + 8]
        cmp eax, edx
        jge .found
    .sloop_next:
        add ecx, 24
        cmp [esi + ecx], dword 0
        je .err
        jmp .sloop
    .found:
        cmp [esi + ecx + 16], dword 1
        jne .sloop_next
        mov eax, [esi + ecx]
        mov [kload_paddr], eax
    pop eax
    pop esi
    pop edx
    mov [kernel_physical_size], edx
    ret
    .err:
        mov ax, 0xe45
        int 0x10
        mov al, '4'
        int 0x10
        jmp $
        
load_elf:
    ;parse elf and relocate each section to it's appropriate location
    ;esi: address of loaded kernel;
    push ebp
    mov ebp, esp
    sub esp, 64; What could i possibly need 16 stack-allocated variables for???
    mov [ebp - 8], esi;file_buffer
    cmp dword [esi], elf_magic
    jne .err
    
    xor ebx, ebx
    mov ebx, [esi + 28]
    lea ebx, [esi + ebx]
    .map_seg_l:
        cmp dword [ebx], 0
        je .exit
        mov esi, [ebx + 8]
        mov edi, [ebx + 4]
        add edi, [ebp - 8]
        mov ecx, [ebx + 20]
        cmp ecx, 0
        je .map_next
        xor edx, edx
        shr ecx, 12
        add dword [kload_paddr], 0x1000
        .mloop:
            add dword [kload_paddr], 0x1000;
            call map_addr
            add edi, 0x1000
            add esi, 0x1000
            ; jmp $
            inc edx
            cmp edx, ecx
            jb .mloop
        .map_next:
            add ebx, 32
            jmp .map_seg_l
    .exit:
    ; jmp $
    mov esi, [ebp - 8]
    mov eax, [esi + 24]; entry address
    ; debug
    add esp, 64
    pop ebp
    ret
    jmp $
    .err:
        mov ah, 0xe
        mov al, 'E'
        int 0x10
        mov al, '3'
        int 0x10
        jmp $

map_addr:
    ;map address in esi to physical address in edi
    push ebp
    mov ebp, esp
    mov [ebp - 8], edi
    mov [ebp - 12], esi
    push edi
    push esi
    
    shr esi, 22
    lea esi, [page_table + esi * 4]
    mov eax, [esi]
    and eax, 0xfffff000
    cmp dword [esi], 0
    jne .map
    
    ;create new page dir entry
    mov eax, [last_allocated_pgtb]
    add eax, 4096
    mov [last_allocated_pgtb], eax
    mov [esi], eax
    or byte [esi], 1
    .map:
        mov esi, eax
        mov eax, [ebp - 12]
        shr eax, 12
        and eax, 0x3ff
        lea esi, [esi + eax * 4]
        mov [esi], edi
        or byte [esi], 1
    pop esi
    pop edi
    pop ebp
    ret
    jmp $
map_pages_id:
    ;edx = page count
    ;eax = addr
    push esi
    push edi
    push edx
    cmp eax, 1 << 22
    je .ret
    mov esi, eax
    .loop:
        cmp edx, 0
        je .ret
        mov edi, esi
        call map_addr
        add esi, 0x1000
        dec edx
        jmp .loop
    .ret:
        pop edx
        pop edi
        pop esi
        ret;
;kernel info
k_info:
    .memory_map_ptr:    dd 0x2000
    .memory_map_count:  dw 0
    .padding:           db 0
    .bpp:               db 0;if depth == 0 then video is text mode
    .xres:              dw 80
    .yres:              dw 25
    .framebuffer_ptr:   dd 0xb8000
    .loaded_modules:    dd initrd
;loaded_modules points to a struct array containing a ptr to the entry point of each pre-loaded module to be executed during startup
; disk_module_struct:
;     .ptr: dd 0
;     .size: dd 0
;     .type:  dd 1
;     .flags: db 0
;     .next_entry: dd fs_module_struct
; fs_module_struct:
;     .ptr: dd 0
;     .size: dd 0
;     .type: dd 2
;     .flags: db 0
;     .next_entry: dd initrc_struct
; initrc_struct:
;     .ptr: dd 0
;     .size: dd 0
;     .type: dd 2
;     .flags: db 1
;     .next_entry: dd 0
initrd:
    .ptr dd 0
    .size dd 0
kernel_file: db "kernel", 0x0, 0x0, "elf"
initrd_file: db "initrd", 0x0, 0x0, "rd"
; disk_module_file: db "idm", 0, 0, 0, 0, 0, "elf"
; fs_module_file: db "ifsm", 0, 0, 0, 0, "elf"
; initrc_file: db "initrc", 0, 0, "conf"
loaded_fat_block: dd 0
last_allocated_pgtb: dd 0x10000
sacrifice1: dd 0
sacrifice0: dd 1
kernel_physical_size: dd 0
kload_paddr: dd 0x20000
elf_magic EQU 0x464c457f
file_buffer EQU 0xa000
root_dir EQU 0x1000
file_table EQU 0xf000
page_table EQU 0x00010000
times 4096-($-$$) db 0