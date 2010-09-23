#include <Boot/DescriptorTables.h>
#include <Common/Strings.h>
#include <Common/Ports.h>
#include <Memory.h>

extern x86::GDTEntry reservedGDTSpace;

extern "C"
{
	void SetIDT(x86::DescriptorTablePointer *);
	void SetTSS(unsigned int);
}

//This is the initial value, described below
virtAddress DescriptorTables::initialGDTValue = 0;
//To start with, the GDT is a pointer to a pre-allocated array. When multiple CPUs are involved, it gets rearranged slightly
x86::GDTEntry *DescriptorTables::gdt = 0;
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
	asm volatile ("sgdt %0" : "=m"(gdtPointer) : : "memory");
	gdt = (x86::GDTEntry *)(gdtPointer.Base);
	initialGDTValue = gdtPointer.Base;
}

void DescriptorTables::installIDT()
{
	extern virtAddress interruptHandlers[];

	idtPointer.Limit = sizeof(idt) - 1;
	idtPointer.Base = (cpuRegister)&idt;

	Memory::Clear(&idt, sizeof(x86::IDTEntry) * 256);

	Ports::WriteByte(0x20, 0x11);	//Initialise primary
	Ports::WriteByte(0xA0, 0x11);	//Intialise secondary
	Ports::WriteByte(0x21, 0x20);	//Primary handles interrupt 32-39
	Ports::WriteByte(0xA1, 0x28);	//Secondary handles interrupt 40-47
	Ports::WriteByte(0x21, 0x04);	//Master is connected to slave via IRQ line 4
	Ports::WriteByte(0xA1, 0x02);	//Slave is connected to master via IRQ line 2
	Ports::WriteByte(0x21, 0x01);	//x86 mode, primary
	Ports::WriteByte(0xA1, 0x01);	//x86 mode, secondary
	Ports::WriteByte(0x21, 0x0);	//Mask, allowing everything
	Ports::WriteByte(0xA1, 0x0);	//Mask, allowing everything

	//I store the list of interrupt handlers in a large array. This saves having to type out a load of method names
	for(uint32 i = 0; i < 256; i++)
		setIDTGate(i, interruptHandlers[i], 0x8, 0x8E);
	asm volatile ("lidt %0" : : "m"(idtPointer));
}

//By this point, I expect physical and virtual memory management to be set up. Heap allocations are therefore no problem
void DescriptorTables::installTSS(uint16 cpuID, cpuRegister esp0, bool installTSS)
{
	//This formula is simply the inverse of the one set in installGDT, adjusted to show the highest index, not the number of entries
	uint64 highestIndex = ((gdtPointer.Limit + 1) / sizeof(x86::GDTEntry)) - 1;
	//tss will need to be allocated dynamically
	x86::TaskStateSegment *newTSS = new x86::TaskStateSegment();
	virtAddress base = (virtAddress)newTSS;
	uint64 limit = base + sizeof(x86::TaskStateSegment);
	uint16 tssIndex = cpuID + 5;
	//I add five because I need to know how many elements there are in the GDT, not just TSSes

    if(installTSS)
        tssIndex = highestIndex + 1;
	if(tssIndex > highestIndex)
	{
		//This is the first time I'm reallocating the GDT. A standard realloc won't work
		if((virtAddress)gdt == initialGDTValue)
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
		gdtPointer.Base = (cpuRegister)gdt;
		//I add one because I need the total number of elements, not the upper bound of the array
		gdtPointer.Limit = (tssIndex + 1) * sizeof(x86::GDTEntry) - 1UL;

		//I really don't like this line, for two reasons
		//1. GCC lectures me about passing a type which isn't equivalent to a pointer as a parameter. I have to point and dereference the
		//   structure.
		//2. This line shouldn't be necessary in the first place. The CPU already knows the address of the GDT pointer. All I do is update
		//   some fields in it.
		asm volatile ("lgdt %0" : : "m"(gdtPointer));
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

	//ESP0 is the stack the kernel should jump to when it comes out of kernel mode. It gets set up by the scheduler
	newTSS->RSP0 = esp0;

	//The IO permission bitmap isn't there, so tell the CPU to ignore it
	newTSS->IOPermissionBitmapOffset = sizeof(x86::TaskStateSegment);
	SetTSS((tssIndex * sizeof(x86::GDTEntry)) | 0x3);
}

void DescriptorTables::setIDTGate(uint32 i, virtAddress function, unsigned short selector, unsigned char flags)
{
	idt[i].FunctionLow = function & 0xFFFF;
	idt[i].FunctionMiddle = (function >> 16) & 0xFFFF;
	idt[i].FunctionHigh = (function >> 32) & 0xFFFFFFFF;
	idt[i].SegmentSelector = selector;
	idt[i].Reserved0 = 0;
	idt[i].Reserved1 = 0;
	idt[i].Flags = flags | 0x60;	// | 0x60 indicates that interrupts can come from ring 3
}

void DescriptorTables::Install()
{
	installGDT();
	installIDT();
}

void DescriptorTables::InstallTSS(virtAddress esp0, uint16 cpuID)
{
	installTSS(cpuID, esp0);
}

/*
* No circular dependancy here, regardless of how it looks. I call AddTSS with esp0 = 0, then the Scheduler class' SetupStack method
* (which internally calls installTSS, passing the current TSS identifier and the new esp0).
*/
void DescriptorTables::AddTSS(virtAddress esp0)
{
    installTSS(0, esp0, true);
}
