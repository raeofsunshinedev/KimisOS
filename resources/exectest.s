segment .text

global _start

_start:
    mov ax, 0x55aa
    int 0x80
end:
    jmp $
segment .data
string: db "Hello, World!", 0x0
segment .bss
resb 4096
