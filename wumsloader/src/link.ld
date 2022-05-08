OUTPUT(loader.elf);

ENTRY(_start);

SECTIONS {
	. = 0x00809000;
	 .text ALIGN(32) : {
          KEEP (*(.crt0))
          KEEP (*(SORT_NONE(.init)))

          *(.text.unlikely .text.*_unlikely .text.unlikely.*)
          *(.text.exit .text.exit.*)
          *(.text.startup .text.startup.*)
          *(.text.hot .text.hot.*)
          *(SORT(.text.sorted.*))
          *(.text .stub .text.* .gnu.linkonce.t.*)
          *(.gnu.warning)
          *(.glink)

          KEEP (*(SORT_NONE(.fini)))
       }

       .rodata : {
          *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
          *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*)

          *(.rodata .rodata.* .gnu.linkonce.r.*)
          *(.rodata1)

          *(.gcc_except_table .gcc_except_table.*)
          *(.gnu_extab*)

          . = ALIGN(4);
          PROVIDE_HIDDEN (__preinit_array_start = .);
          KEEP (*(.preinit_array))
          PROVIDE_HIDDEN (__preinit_array_end = .);

          PROVIDE_HIDDEN (__init_array_start = .);
          KEEP (*crtbegin.o(.ctors))
          KEEP (*crtbegin?.o(.ctors))
          KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
          KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
          KEEP (*(SORT(.ctors.*)))
          KEEP (*(.ctors))
          PROVIDE_HIDDEN (__init_array_end = .);

          PROVIDE_HIDDEN (__fini_array_start = .);
          KEEP (*crtbegin.o(.dtors))
          KEEP (*crtbegin?.o(.dtors))
          KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
          KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
          KEEP (*(SORT(.dtors.*)))
          KEEP (*(.dtors))
          PROVIDE_HIDDEN (__fini_array_end = .);

          KEEP (*(.jcr))
          *(.fixup)
          *(.got1)
          *(.got2)
          *(.branch_lt)
          *(.got)
          *(.plt)
          *(.tm_clone_table)
       }

       .eh_frame : {
          *(.eh_frame_hdr)
          *(.eh_frame_entry .eh_frame_entry.*)

          KEEP (*(.eh_frame))
          *(.eh_frame.*)
       }

       .data : {
          *(.data .data.* .gnu.linkonce.d.*)
          SORT(CONSTRUCTORS)
          *(.data1)
          *(.sdata .sdata.* .gnu.linkonce.s.*)
       }

       .bss (NOLOAD) : {
          *(.dynsbss)
          *(.sbss .sbss.* .gnu.linkonce.sb.*)
          *(.scommon)

          *(.dynbss)
          *(.bss .bss.* .gnu.linkonce.b.*)
          *(COMMON)
       }
	/DISCARD/ : {
		*(*);
	}
}

ASSERT((SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata) + SIZEOF(.eh_frame) + SIZEOF(.bss)) < 0x0D0000, "Memory overlapping with modules.");
