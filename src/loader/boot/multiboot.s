.code32
.set MB_ALIGN_MODS, 1 << 0
.set MB_MEMINFO,    1 << 1
.set MB_FLAGS,      MB_ALIGN_MODS | MB_MEMINFO
.set MB_MAGIC,      0x1BADB002
.set MB_CHECKSUM,   -(MB_MAGIC + MB_FLAGS)

.set KERNEL_VIRTUAL_BASE,	  0xf0000000
.set KERNEL_PAGE_NUMBER,	  (KERNEL_VIRTUAL_BASE >> 22)

.section .multiboot

.align 16
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECKSUM

.section .text

.global multiboot_entry
.align 16
multiboot_entry:
    cli

    movl $pm_initial_pd - KERNEL_VIRTUAL_BASE, %ecx
    movl %ecx, %cr3

	movl %cr4, %ecx
	orl $0x00000010, %ecx
	movl %ecx, %cr4

	movl %cr0, %ecx
	orl $0x80000000, %ecx
	movl %ecx, %cr0
    
    movl %ebx, mb_info_ptr

    lgdt (gdtr32)
    ljmp $0x08, $.gdt32_loaded

.gdt32_loaded:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $stack_top, %esp
    movl $stack_top, %ebp

    // enabling fpu+sse
    
    mov %cr0, %eax
    and $0xfffffffd, %eax // turning off CR0.EM 
    or $0x00000022, %eax // turning on CR0.MP and CR0.NE
    mov %eax, %cr0
    fninit
    mov %cr4, %eax
    or $0x00000600, %eax // enabling CR4.OSFXSR and CR4.OSXMMEXCPT
    mov %eax, %cr4
    
    call _init
    call loader_main

.global long_enter
.align 16
long_enter:
    cli
    movl $entry_store, %esi

    ljmp $0x08, $.long_enter_trampoline - 0xf0000000
.long_enter_trampoline:
    movl 8(%esp), %eax
    movl %eax, 4(%esi)
    movl 4(%esp), %eax
    movl %eax, (%esi)

    movl (%esi), %edi
    movl 4(%esi), %esi

    movl 12(%esp), %ecx

    movl %cr0, %eax
    andl $~0x80000000, %eax
    movl %eax, %cr0

    movl %ecx, %cr3
    
    movl $0xC0000080, %ecx
    rdmsr
    orl $(1 << 8), %eax
    movl $0xC0000080, %ecx
    wrmsr

    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0

    lgdt (gdtr64 - 0xf0000000)
    ljmp $0x08, $.long_cont - 0xf0000000

.long_cont:
.code64
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    mov %esi, %eax
    shl $32, %rax
    xor %rbx, %rbx
    mov %edi, %ebx 
    add %rbx, %rax

    movabs $0x4000a00000, %rsp
    jmp *%rax

.code32
.align 16
gdt32:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdtr32:
    .word (gdtr32 - gdt32 - 1)
    .long gdt32
.align 16
gdt64:
	.quad 0x0000000000000000
	.quad 0x00209A0000000000
	.quad 0x0020920000000000
gdtr64:
	.word (gdtr64 - gdt64 - 1)
	.long gdt64 - 0xf0000000

.global mb_info_ptr
.align 4
mb_info_ptr:
    .long 0

.align 8
entry_store:
    .quad 0


.section .data
.global pm_initial_pd
.align 0x1000
pm_initial_pd:
	.long 0x00000087
	.long 0x00400087
	.long 0x00800087
	.long 0x00c00087
    .fill (KERNEL_PAGE_NUMBER - 4), 4, 0
    .long 0x00000087
    .long 0x00400087
    .long 0x00800087
	.long 0x00c00087
    .fill (1024 - KERNEL_PAGE_NUMBER - 4), 4, 0