bits 32

extern kmain
section .text
global _start
;TODO: re-write stub to be compatible with any bootloader
_start:
    cld
    mov esp, stack_top
    mov ebp, stack_top
    push esi
    ; jmp $
    call kmain
    jmp $
    
section .bss
stack_bottom: resb 0x10000
stack_top: