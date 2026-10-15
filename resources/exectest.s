segment .text

global _start

_start:
    mov eax, 7
    int 0x80

    push eax
    mov edx, eax
    .ext:
    cmp eax, 3
    jne end
    mov ebx, string
    mov byte [ebx + 13], 0x41
    .tst:
    mov eax, 1
    mov ebx, string
    int 0x80
    mov eax, 0
    ; mov ebx, edx
    pop ebx
    int 0x80
end:
    mov eax, 3
    jmp _start.ext
    mov eax, 1
    mov ebx, string
    int 0x80
    pop ebx
    mov eax, 0
    int 0x80
    jmp $
segment .data
string: db "Hello, World! ", 0x0
segment .bss
resb 4096
