#include <Boot/DescriptorTables.h>
#include <Common/Strings.h>
#include <Common/Ports.h>
#include <Memory.h>

extern x86::GDTEntry reservedGDTSpace;

extern "C"
{
	void SetGDT(x86::DescriptorTablePointer *);
	void SetIDT(x86::DescriptorTablePointer *);
	void SetTSS(unsigned int);

	void ISR0();
	void ISR1();
	void ISR2();
	void ISR3();
	void ISR4();
	void ISR5();
	void ISR6();
	void ISR7();
	void ISR8();
	void ISR9();
	void ISR10();
	void ISR11();
	void ISR12();
	void ISR13();
	void ISR14();
	void ISR15();
	void ISR16();
	void ISR17();
	void ISR18();
	void ISR19();
	void ISR20();
	void ISR21();
	void ISR22();
	void ISR23();
	void ISR24();
	void ISR25();
	void ISR26();
	void ISR27();
	void ISR28();
	void ISR29();
	void ISR30();
	void ISR31();

	void IRQ0();
	void IRQ1();
	void IRQ2();
	void IRQ3();
	void IRQ4();
	void IRQ5();
	void IRQ6();
	void IRQ7();
	void IRQ8();
	void IRQ9();
	void IRQ10();
	void IRQ11();
	void IRQ12();
	void IRQ13();
	void IRQ14();
	void IRQ15();

	void ISR48();
	void ISR64();
}

//To start with, the GDT is a pointer to a pre-allocated array. When multiple CPUs are involved, it gets rearranged slightly
x86::GDTEntry *DescriptorTables::gdt = &reservedGDTSpace;
//Update the values of Base and Limit when I install and remove a TSS
x86::DescriptorTablePointer DescriptorTables::gdtPointer;

x86::IDTEntry DescriptorTables::idt[256];
x86::DescriptorTablePointer DescriptorTables::idtPointer;

DescriptorTables::DescriptorTables()
{
}

DescriptorTables::~DescriptorTables()
{
}

void DescriptorTables::installGDT()
{
	unsigned int accessFlags[5] = {0x0, 0x9A, 0x92, 0xFA, 0xF2};

	gdt[0].LimitLow = gdt[0].BaseLow = gdt[0].BaseMiddle = gdt[0].AccessFlags = gdt[0].Granularity = gdt[0].BaseHigh = 0;
	for(unsigned char i = 1; i < 5; i++)
	{
		gdt[i].LimitLow = 0xFFFF;
		gdt[i].BaseLow = 0;
		gdt[i].BaseMiddle = 0;
		gdt[i].AccessFlags = accessFlags[i];
		gdt[i].Granularity = 0xCF;
		gdt[i].BaseHigh = 0;
	}
	gdtPointer.Limit = sizeof(x86::GDTEntry) * 6 - 1;
	gdtPointer.Base = (unsigned int)gdt;
	SetGDT(&gdtPointer);
}

void DescriptorTables::installIDT()
{
	idtPointer.Limit = sizeof(idt) - 1;
	idtPointer.Base = (unsigned int)&idt;

	Memory::Clear(&idt, sizeof(x86::IDTEntry) * 256);

	Ports::WriteByte(0x20, 0x11);	//Initialise primary
	Ports::WriteByte(0xA0, 0x11);	//Intialise secondary
	Ports::WriteByte(0x21, 0x20);	//Primary handles interrupt 32-39
	Ports::WriteByte(0xA1, 0x28);	//Secondary handles interrupt 40-47
	Ports::WriteByte(0x21, 0x04);
	Ports::WriteByte(0xA1, 0x02);
	Ports::WriteByte(0x21, 0x01);
	Ports::WriteByte(0xA1, 0x01);
	Ports::WriteByte(0x21, 0x0);	//Mask, allowing everything
	Ports::WriteByte(0xA1, 0x0);	//Mask, allowing everything

	ISREntry(0);
	ISREntry(1);
	ISREntry(2);
	ISREntry(3);
	ISREntry(4);
	ISREntry(5);
	ISREntry(6);
	ISREntry(7);
	ISREntry(8);
	ISREntry(9);
	ISREntry(10);
	ISREntry(11);
	ISREntry(12);
	ISREntry(13);
	ISREntry(14);
	ISREntry(15);
	ISREntry(16);
	ISREntry(17);
	ISREntry(18);
	ISREntry(19);
	ISREntry(20);
	ISREntry(21);
	ISREntry(22);
	ISREntry(23);
	ISREntry(24);
	ISREntry(25);
	ISREntry(26);
	ISREntry(27);
	ISREntry(28);
	ISREntry(29);
	ISREntry(30);
	ISREntry(31);

	//Links to assembly code, covering the system call interfaces
	ISREntry(48);
	ISREntry(64);

	IRQEntry(0);
	IRQEntry(1);
	IRQEntry(2);
	IRQEntry(3);
	IRQEntry(4);
	IRQEntry(5);
	IRQEntry(6);
	IRQEntry(7);
	IRQEntry(8);
	IRQEntry(9);
	IRQEntry(10);
	IRQEntry(11);
	IRQEntry(12);
	IRQEntry(13);
	IRQEntry(14);
	IRQEntry(15);

	SetIDT(&idtPointer);
}

//By this point, I expect physical and virtual memory management to be set up. Heap allocations are therefore no problem
void DescriptorTables::installTSS(unsigned int cpuID, unsigned int esp0, bool installTSS)
{
	//This formula is simply the inverse of the one set in installGDT, adjusted to show the highest index, not the number of entries
	unsigned int highestIndex = ((gdtPointer.Limit + 1) / sizeof(x86::GDTEntry)) - 1;
	//tss will need to be allocated dynamically
	x86::TaskStateSegment *tss = new x86::TaskStateSegment();
	unsigned int base = (unsigned int)tss;
	unsigned int limit = base + sizeof(x86::TaskStateSegment);
	//I add five because I need to know how many elements there are in the GDT, not just TSSes
	unsigned int tssIndex = cpuID + 5;

    if(installTSS)
        tssIndex = highestIndex + 1;
	if(tssIndex > highestIndex)
	{
		//This is the first time I'm reallocating the GDT. A standard realloc won't work
		if(gdt == &reservedGDTSpace)
		{
			x86::GDTEntry *newGdt = new x86::GDTEntry[tssIndex + 1];

			//I add one because I need the total number of elements, not the upper bound of the array
			Memory::Copy(newGdt, (const void *)gdt, (highestIndex + 1) * sizeof(x86::GDTEntry));
			gdt = newGdt;
		}
		else
		{
			//This does the same as the previous case, but more cleanly
			x86::GDTEntry *newGdt = (x86::GDTEntry *)MemoryManagement::Heap::GetKernelHeap()->
				Reallocate(gdt, sizeof(x86::GDTEntry) * tssIndex);

			if(newGdt != 0)
				gdt = newGdt;
			else
				asm volatile ("cli; hlt" : : "a"(0xDEAD));
		}
		gdtPointer.Base = (unsigned int)gdt;
		//I add one because I need the total number of elements, not the upper bound of the array
		gdtPointer.Limit = (tssIndex + 1) * sizeof(x86::GDTEntry) - 1;

		//I really don't like this line, for two reasons
		//1. GCC lectures me about passing a type which isn't equivalent to a pointer as a parameter. I have to point and dereference the
		//   structure.
		//2. This line shouldn't be necessary in the first place. The CPU already knows the address of the GDT pointer. All I do is update
		//   some fields in it.
		asm volatile ("lgdt (%%eax)" : : "a"(&gdtPointer));
	}
	else if(tssIndex < highestIndex)
		//Removal of TSSes is not supported. If I wanted to shut down a processor, I would implement it
		asm volatile ("cli; hlt" : : "a"(0xDEAD));
	//Null segment, plus user and kernel code and data
	gdt[tssIndex].LimitLow = limit & 0xFFFF;
	gdt[tssIndex].BaseLow = base & 0xFFFF;
	gdt[tssIndex].BaseMiddle = (base >> 16) & 0xFF;
	gdt[tssIndex].AccessFlags = 0xE9;
	gdt[tssIndex].Granularity = (limit >> 16) & 0xF;
	gdt[tssIndex].BaseHigh = (base >> 24) & 0xFF;

	//0x10 is the third GDT entry - the kernel-mode data segment
	tss->SS0 = 0x10;
	//ESP0 is the stack the kernel should jump to when it comes out of kernel mode. It gets set up by the scheduler
	tss->ESP0 = esp0;

	//The code segment offset of kernel-mode code, ORed with the privilege level (3)
	tss->CS = 0x8 | 0x3;
	tss->SS = tss->DS = tss->ES = tss->FS = tss->GS = 0x10 | 0x3;	//Kernel-mode data OR the privilege level
	//The IO permission bitmap isn't there, so tell the CPU to ignore it
	tss->IOPermissionBitmapOffset = sizeof(x86::TaskStateSegment);
	SetTSS((tssIndex * sizeof(x86::GDTEntry)) | 0x3);
}

void DescriptorTables::setIDTGate(unsigned char i, unsigned int function, unsigned short selector, unsigned char flags)
{
	idt[i].FunctionLow = function & 0xFFFF;
	idt[i].FunctionHigh = (function >> 16) & 0xFFFF;
	idt[i].SegmentSelector = selector;
	idt[i].Reserved = 0;
	idt[i].Flags = flags | 0x60;	// | 0x60 indicates that interrupts can come from ring 3
}

void DescriptorTables::Install()
{
	installGDT();
	installIDT();
}

void DescriptorTables::InstallTSS(unsigned int esp0, unsigned int cpuID)
{
	installTSS(cpuID, esp0);
}

/*
* No circular dependancy here, regardless of how it looks. I call AddTSS with esp0 = 0, then the Scheduler class' SetupStack method
* (which internally calls installTSS, passing the current TSS identifier and the new esp0).
*/
void DescriptorTables::AddTSS(unsigned int esp0)
{
    installTSS(0, esp0, true);
}
