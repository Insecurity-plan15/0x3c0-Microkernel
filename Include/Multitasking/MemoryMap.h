#ifndef MemoryMap_H
#define MemoryMap_H

#include <LinkedList.h>
#include <Multitasking/Paging.h>

//The kernel heap is placed at 3.25 GiB
#define KernelHeapStart	0xD0000000
//The heap is 256 MiB large, ending at 0xE0000000
#define KernelHeapEnd		0xDFFFFFFF

#define UserHeapStart	0xE0000000
#define UserHeapEnd		0xEFFFFFFF

namespace MemoryZonePurpose
{
	const unsigned int Kernel = 0x1;
	const unsigned int Heap = 0x2;
	const unsigned int Stack = 0x4;
	const unsigned int Paged = 0x8;
	const unsigned int User = 0x10;
}

//A memory zone is a region of memory. It's just plain old data
struct MemoryZone
{
public:
	unsigned int Start;
	unsigned int End;
	unsigned int Purpose;
	MemoryZone(unsigned int s, unsigned int e, unsigned int purp)
	{
		Start = s;
		End = e;
		Purpose = purp;
	}

	~MemoryZone()
	{
	}
};

//I reuse the LinkedList class, because it's just a special collection
class MemoryMap
	: public LinkedList<MemoryZone *>
{
public:
	MemoryMap();
	~MemoryMap();
	static MemoryMap *GetDefaultMap();
	MemoryMap *Clone();
};

#endif
