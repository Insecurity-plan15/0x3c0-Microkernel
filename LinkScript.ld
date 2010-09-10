OUTPUT_FORMAT(elf64-x86-64)
ENTRY(asmEntry)
LINEAR_ADDRESS = 0x100000;
VIRTUAL_ADDRESS = 0xFFFF800000000000;

SECTIONS
{
	. = LINEAR_ADDRESS;

	textStart = .;

	.pmode ALIGN(0x1000) :
	{
		*(.multiboot*)
		*(.pmode.gdt*)
		*(.pmode.text*)
		*(.pmode.bss*)
	}


	. += VIRTUAL_ADDRESS;

	.text ALIGN(0x1000) : AT(ADDR(.text) - VIRTUAL_ADDRESS)
	{
		*(.text)
		*(.rodata*)
	}
	textEnd = .;

	dataStart = .;
	.data ALIGN(0x1000) : AT(ADDR(.data) - VIRTUAL_ADDRESS)
	{
		*(.data)
		constructorStart = .;
		*(.ctor*)
		constructorEnd = .;
	}
	dataEnd = .;

	bssStart = .;
	.bss ALIGN(0x1000) : AT(ADDR(.bss) - VIRTUAL_ADDRESS)
	{
		startBss = .;
		*(COMMON)
		*(.bss)
		endBss = .;
	}
	bssEnd = .;

	/DISCARD/ :
	{
		*(.eh_frame)
	}
	end = .;
}
