#ifndef DescriptorTables_H
#define DescriptorTables_H

#include <Multitasking/Scheduler.h>

namespace x86
{
	struct GDTEntry
	{
		unsigned short LimitLow;
		unsigned short BaseLow;
		unsigned char BaseMiddle;
		unsigned char AccessFlags;
		unsigned char Granularity;
		unsigned char BaseHigh;
	} __attribute__((packed));

	struct IDTEntry
	{
		unsigned short FunctionLow;
		unsigned short SegmentSelector;
		unsigned char Reserved;
		unsigned char Flags;
		unsigned short FunctionHigh;
	} __attribute__((packed));

	struct DescriptorTablePointer
	{
		unsigned short Limit;
		unsigned int Base;		//Address of the first entry
	} __attribute__((packed));

	struct TaskStateSegment
	{
		//If hardware multitasking is used, points to the next TSS
		unsigned short Link;
		unsigned short Reserved0;
		unsigned int ESP0;
		unsigned short SS0;
		unsigned short Reserved1;

		unsigned int ESP1;
		unsigned short SS1;
		unsigned short Reserved2;

		unsigned int ESP2;
		unsigned short SS2;
		unsigned short Reserved3;

		unsigned int CR3;
		unsigned int EIP;
		unsigned int EFlags;

		unsigned int EAX;
		unsigned int ECX;
		unsigned int EDX;
		unsigned int EBX;

		unsigned int ESP;
		unsigned int EBP;

		unsigned int ESI;
		unsigned int EDI;

		unsigned short ES;
		unsigned short Reserved4;
		unsigned short CS;
		unsigned short Reserved5;
		unsigned short SS;
		unsigned short Reserved6;
		unsigned short DS;
		unsigned short Reserved7;
		unsigned short FS;
		unsigned short Reserved8;
		unsigned short GS;
		unsigned short Reserved9;

		unsigned short LDTR;
		unsigned int Reserved10;
		unsigned short IOPermissionBitmapOffset;
	} __attribute__((packed));
}

class DescriptorTables
{
friend class Scheduler;
private:
	static x86::GDTEntry *gdt;
	static x86::DescriptorTablePointer gdtPointer;

	static x86::IDTEntry idt[256];
	static x86::DescriptorTablePointer idtPointer;

	//static x86::TaskStateSegment tss;

	DescriptorTables();
	~DescriptorTables();
	static void installGDT();
	#define ISREntry(n)	setIDTGate(n, (unsigned int)ISR##n, 0x8, 0x8E)
	#define IRQEntry(n) setIDTGate(n+32, (unsigned int)IRQ##n, 0x8, 0x8E)
	static void installIDT();
	static void installTSS(unsigned int cpuID, unsigned int esp0);
	static void setIDTGate(unsigned char i, unsigned int function, unsigned short selector, unsigned char flags);
public:
	static void Install();
	static void InstallTSS(unsigned int esp0, unsigned int cpuID);
};

#endif
