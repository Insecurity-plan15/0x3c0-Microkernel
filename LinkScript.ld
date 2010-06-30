ENTRY(asmEntry)
OUTPUT_FORMAT(elf32-i386)
VIRTUAL_ADDRESS = 0xF0000000;

SECTIONS
{
	/* Kernel resides at the 3 GiB mark */
	. = VIRTUAL_ADDRESS + 0x100000;

	.text : AT(ADDR(.text) - VIRTUAL_ADDRESS)
	{
		*(.text)
		*(.rodata*)
	}
	.data ALIGN(0x1000) : AT(ADDR(.data) - VIRTUAL_ADDRESS)
	{
		constructorStart = .;
		*(.ctor*)
		constructorEnd = .;
		*(.data)
	}
	.bss : AT(ADDR(.bss) - VIRTUAL_ADDRESS)
	{
		startBss = .;
		*(COMMON)
		*(.bss)
		endBss = .;
	}
	end = .;
}
