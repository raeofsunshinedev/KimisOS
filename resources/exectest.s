segment .text
_start:
    mov edi, 0x8b000
    mov esi, string
    xor ebx, ebx
    mov ah, 0x0f
printstr:
    lodsb
    cmp al, 0
    je end
    mov [edi + ebx], ax
    inc bx
    jmp printstr
end:
    jmp $
segment .data
string: db "Hello, World!", 0x0