#include <Multitasking/MemoryMap.h>
#include <Multitasking/Paging.h>

MemoryMap::MemoryMap()
{
}

MemoryMap::~MemoryMap()
{
	if(this != MemoryMap::GetDefaultMap())
		for(LinkedListNode<MemoryZone *> *nd = this->First; nd != 0; nd = nd->Next)
			delete nd->Value;
}

MemoryMap *MemoryMap::GetDefaultMap()
{
	MemoryMap *map = new MemoryMap();

	//Below 0xC0000000 is user space
	map->Add(new MemoryZone(0x0, 0xAFFFFFFF, MemoryZonePurpose::User));
	//0xB0000000 : 0xBFFFFFFF is the user heap
	map->Add(new MemoryZone(UserHeapStart, 0, MemoryZonePurpose::User | MemoryZonePurpose::Heap));
	//0xC0000000 : 0xCFFFFFFF is kernel space
	map->Add(new MemoryZone(0xC0000000, 0xCFFFFFFF, MemoryZonePurpose::Kernel));
	//0xD0000000 : 0xF0000000 is the heap. The minimum size is 0, but when it gets created it moves in page sizes
	map->Add(new MemoryZone(KernelHeapStart, 0, MemoryZonePurpose::Kernel | MemoryZonePurpose::Heap));

	return map;
}

MemoryMap *MemoryMap::Clone()
{
	MemoryMap *map = new MemoryMap();

	for(LinkedListNode<MemoryZone *> *nd = this->First; nd != 0 && nd->Value != 0; nd = nd->Next)
		if((nd->Value->Purpose & MemoryZonePurpose::Kernel) == MemoryZonePurpose::Kernel)
			//Entries related to the kernel are expected to remain consistent across every address space
			//Add a pointer
			map->Add(nd->Value);
		else
			//Otherwise, just do a straight clone of the entry
			map->Add(new MemoryZone(nd->Value->Start, nd->Value->End, nd->Value->Purpose));
	return map;
}
