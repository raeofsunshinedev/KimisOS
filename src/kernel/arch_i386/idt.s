global _isr0
global _isr1
global _isr2;
global _isr3;
global _isr4;
global _isr5;
global _isr6
global _isr7
global _isr8
global _isr9
global _isr10
global _isr11
global _isr12
global _isr13
global _isr14
global _isr15
global _isr16
global _isr17
global _isr18
global _isr19
global _isr20
global _isr21
global _isr22
global _isr23
global _isr24
global _isr25
global _isr26
global _isr27
global _isr28
global _isr29
global _isr30
global _isr31

global load_idt
section .text
load_idt:
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    lidt [eax]
    pop ebp
    ret

_isr0:
    cli
    push dword 0
    push dword 0
    jmp _isr_common_stub
_isr1:
    cli
    push dword 0
    push dword 1
    jmp _isr_common_stub
_isr2:
    cli
    push dword 0
    push dword 2
    jmp _isr_common_stub
_isr3:
    cli
    push dword 0
    push dword 3
    jmp _isr_common_stub
_isr4:
    cli
    push dword 0
    push dword 4
    jmp _isr_common_stub
_isr5:
    cli
    push dword 0
    push dword 5
    jmp _isr_common_stub
_isr6:
    cli
    xchg bx, bx
    push dword 0
    push dword 6
    jmp _isr_common_stub
_isr7:
    cli
    push dword 0
    push dword 7
    jmp _isr_common_stub
_isr8:
    cli
    push dword 8
    jmp _isr_common_stub
_isr9:
    cli
    push dword 0
    push dword 9
    jmp _isr_common_stub
_isr10:
    cli
    push dword 10
    jmp _isr_common_stub
_isr11:
    cli
    push dword 11
    jmp _isr_common_stub
_isr12:
    cli
    push dword 12
    jmp _isr_common_stub
_isr13:
    cli
    push dword 13
    jmp _isr_common_stub
_isr14:
    cli
    push dword 14
    jmp _isr_common_stub
_isr15:
    cli
    push dword 0
    push dword 15
    jmp _isr_common_stub
_isr16:
    cli
    push dword 0
    push dword 16
    jmp _isr_common_stub
_isr17:
    cli
    push dword 0
    push dword 17
    jmp _isr_common_stub
_isr18:
    cli
    push dword 0
    push dword 18
    jmp _isr_common_stub
_isr19:
    cli
    push dword 0
    push dword 19
    jmp _isr_common_stub
_isr20:
    cli
    push dword 0
    push dword 20
    jmp _isr_common_stub
_isr21:
    cli
    push dword 0
    push dword 21
    jmp _isr_common_stub
_isr22:
    cli
    push dword 0
    push dword 22
    jmp _isr_common_stub
_isr23:
    cli
    push dword 0
    push dword 23
    jmp _isr_common_stub
_isr24:
    cli
    push dword 0
    push dword 24
    jmp _isr_common_stub
_isr25:
    cli
    push dword 0
    push dword 25
    jmp _isr_common_stub
_isr26:
    cli
    push dword 0
    push dword 26
    jmp _isr_common_stub
_isr27:
    cli
    push dword 0
    push dword 27
    jmp _isr_common_stub
_isr28:
    cli
    push dword 0
    push dword 28
    jmp _isr_common_stub
_isr29:
    cli
    push dword 29
    jmp _isr_common_stub
_isr30:
    cli
    push dword 30
    jmp _isr_common_stub
_isr31:
    cli
    push dword 0
    push dword 31
    jmp _isr_common_stub

extern _isr_handler
global _isr_common_stub

_isr_common_stub:
    pushad
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp;push argument
    call _isr_handler
    pop esp
    mov esp, eax
    pop gs
    pop fs
    pop es
    pop ds
    popad
    add esp, 8
    iret

extern _irq_handler
global _irq_common_stub

global _irq0
global _irq1
global _irq2
global _irq3
global _irq4
global _irq5
global _irq6
global _irq7
global _irq8
global _irq9
global _irq10
global _irq11
global _irq12
global _irq13
global _irq14
global _irq15
global _syscall
_irq0:
    cli
    ; jmp $
    push dword 0
    push dword 32
    jmp _irq_common_stub
_irq1:
    cli
    push dword 0
    push dword 33
    jmp _irq_common_stub
_irq2:
    cli
    push dword 0
    push dword 34
    jmp _irq_common_stub
_irq3:
    cli
    push dword 0
    push dword 35
    jmp _irq_common_stub
_irq4:
    cli
    push dword 0
    push dword 36
    jmp _irq_common_stub
_irq5:
    cli
    push dword 0
    push dword 37
    jmp _irq_common_stub
_irq6:
    cli
    push dword 0
    push dword 38
    jmp _irq_common_stub
_irq7:
    cli
    push dword 0
    push dword 39
    jmp _irq_common_stub
_irq8:
    cli
    push dword 0
    push dword 40
    jmp _irq_common_stub
_irq9:
    cli
    push dword 0
    push dword 41
    jmp _irq_common_stub
_irq10:
    cli
    push dword 0
    push dword 42
    jmp _irq_common_stub
_irq11:
    cli
    push dword 0
    push dword 43
    jmp _irq_common_stub
_irq12:
    cli
    push dword 0
    push dword 44
    jmp _irq_common_stub
_irq13:
    cli
    push dword 0
    push dword 45
    jmp _irq_common_stub
_irq14:
    cli
    push dword 0
    push dword 46
    jmp _irq_common_stub
_irq15:
    cli
    push dword 0
    push dword 47
    jmp _irq_common_stub

_syscall:
    cli
    push dword 0
    push dword 0x80

_irq_common_stub:
    pushad
    push ds
    push es
    push fs
    push gs
    
    push esp;push argument
    call _irq_handler
    pop esp
    mov esp, eax
    pop gs
    pop fs
    pop es
    pop ds
    popad
    add esp, 8
    ; sti
    iret

global panic_hold

panic_hold:
    
    push ebp
    mov ebp, esp
    
    mov esp, [ebp + 8]
    
    pop gs
    pop fs
    pop es
    pop ds
    popad
    jmp $


global get_gdt

get_gdt:
    sgdt [gdt_desc]
    mov eax, gdt_desc
    ret

section .data
align 4
gdt_desc:
    dw 0; limit
    dd 0; ptr