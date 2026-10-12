segment .text

global _start

_start:
    mov edi, 0xb8000
    mov esi, string
    xor ebx, ebx
    mov ah, 0x0f
printstr:
    lodsb
    cmp al, 0
    je end
    mov [edi + ebx * 2], ax
    inc bx
    jmp printstr
end:
    xor eax, eax
    mov ax, ss
    int 0
    xor ecx, ecx
    div ecx
    mov [eax], 81
    jmp $
segment .data
string: db "Hello, World!", 0x0
segment .bss
resb 4096
