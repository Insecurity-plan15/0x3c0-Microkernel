ENTRY(Main)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
	/* All processes shall reside at virtual address 0x100000 (1 MiB)*/
	. = 0x100000;

	.text :
	{
		*(.text)
		*(.rodata*)
	}
	.data ALIGN(0x1000) :
	{
		constructorStart = .;
		*(.ctor*)
		constructorEnd = .;
		*(.data)
	}
	.bss :
	{
		startBss = .;
		*(COMMON)
		*(.bss)
		endBss = .;
	}
	end = .;
}
