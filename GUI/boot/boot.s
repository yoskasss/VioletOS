; VioletOS - Multiboot bootloader entry point
[BITS 32]

section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00000007
    dd -(0x1BADB002 + 0x00000007)
    dd 0 ; header_addr
    dd 0 ; load_addr
    dd 0 ; load_end_addr
    dd 0 ; bss_end_addr
    dd 0 ; entry_addr
    dd 0 ; mode_type (0 = linear graphics)
    dd 800 ; width
    dd 600 ; height
    dd 32 ; depth

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    push ebx
    push eax
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 65536
stack_top:
