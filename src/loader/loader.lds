ENTRY(multiboot_entry)

SECTIONS {
    . = 0xf0100000;

    .text ALIGN(4K) : AT(ADDR(.text) - 0xf0000000) {
		*(.multiboot)
		*(.text)
	}
  
	.init ALIGN(4K) : AT(ADDR(.init) - 0xf0000000) {
		*(.init)
	}
	
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xf0000000) {
		*(.rodata)
	}

	.data ALIGN(4K) : AT(ADDR(.data) - 0xf0000000) {
		*(.data)
	}

	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xf0000000) {
		*(.bss)
		*(COMMON)
	}

    . = ALIGN(4K);
    _end = .;
    PROVIDE (end = .);
    . = . + 0x200000;
    PROVIDE (stack_bottom = .);
    . = . + 0x100000;
    PROVIDE (stack_top = .);
}
