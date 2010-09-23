#ifndef DescriptorTables_H
#define DescriptorTables_H

#include <Typedefs.h>
#include <Multitasking/Scheduler.h>

namespace x86
{
	struct GDTEntry
	{
		uint16 LimitLow;
		uint16 BaseLow;
		uint8 BaseMiddle;
		uint8 AccessFlags;
		uint8 Granularity;
		uint8 BaseHigh;
	} __attribute__((packed));

	struct IDTEntry
	{
		uint16 FunctionLow;
		uint16 SegmentSelector;
		uint8 Reserved0;
		uint8 Flags;
		uint16 FunctionMiddle;
		uint32 FunctionHigh;
		uint32 Reserved1;
	} __attribute__((packed));

	struct DescriptorTablePointer
	{
		unsigned short Limit;
		cpuRegister Base;		//Address of the first entry
	} __attribute__((packed));

	struct TaskStateSegment
	{
		uint32 Reserved0;
		virtAddress RSP0;
		virtAddress RSP1;
		virtAddress RSP2;
		uint64 Reserved1;
		uint64 IST[7];
		uint64 Reserved2;
		uint16 Reserved3;
		uint16 IOPermissionBitmapOffset;
	} __attribute__((packed));
}

class DescriptorTables
{
friend class Scheduler;
private:
	static virtAddress initialGDTValue;
	static x86::GDTEntry *gdt;
	static x86::DescriptorTablePointer gdtPointer;

	static x86::IDTEntry idt[256];
	static x86::DescriptorTablePointer idtPointer;

	static x86::TaskStateSegment tss;

	DescriptorTables();
	~DescriptorTables();
	static void installGDT();
	static void installIDT();
	//If newCPU is true, cpuID is ignored and a new TSS is created at the end of the GDT
	static void installTSS(uint16 cpuID, cpuRegister esp0, bool newCPU = false);
	static void setIDTGate(uint32 i, virtAddress function, unsigned short selector, unsigned char flags);
public:
	static void Install();
	static void InstallTSS(virtAddress esp0, uint16 cpuID);
	static void AddTSS(virtAddress esp0);
};

#endif
