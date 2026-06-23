; VioletOS - IDT load and ISR/IRQ stubs
[BITS 32]

global idt_load
extern isr_handler
extern irq_dispatch

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    sti
    ret

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push 0
    push %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push %1
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7

; Double fault: do not call C (stack may be corrupt)
global isr8
isr8:
    cli
.hang8:
    hlt
    jmp .hang8

ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

%macro IRQ 1
global isr%1
isr%1:
    cli
    push 0
    push %1
    jmp irq_common_stub
%endmacro

IRQ 32
IRQ 33
IRQ 34
IRQ 35
IRQ 36
IRQ 37
IRQ 38
IRQ 39
IRQ 40
IRQ 41
IRQ 42
IRQ 43
IRQ 44
IRQ 45
IRQ 46
IRQ 47

isr_common_stub:
    pusha
    push dword [esp + 32]
    call isr_handler
    add esp, 4
    popa
    add esp, 8
    iret

irq_common_stub:
    pusha
    push dword [esp + 32]
    call irq_dispatch
    add esp, 4
    popa
    add esp, 8
    iret
