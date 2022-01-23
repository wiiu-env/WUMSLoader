OUTPUT(loader.elf);

ENTRY(_start);

SECTIONS {
	. = 0x00809000;
	.text : {
		*(.kernel_code*);
		*(.text*);
		/* Tell linker to not garbage collect this section as it is not referenced anywhere */
		KEEP(*(.kernel_code*));
	}	
	.sdata : {
		*(.sdata*)
	}
	.data : {
		*(.data*)
	}
	.rodata : {
		*(.rodata*)
	}
	.sbss : {
		*(.sbss*)
	}
	.bss : {
		*(.bss*)
	}
	/DISCARD/ : {
		*(*);
	}
}

ASSERT((SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.sdata) + SIZEOF(.rodata) + SIZEOF(.sbss) + SIZEOF(.bss)) < 0x77000, "Memory overlapping with modules.");
