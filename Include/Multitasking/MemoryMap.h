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

#endif
