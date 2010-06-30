#include <Multitasking/Paging.h>
#include <MemoryAllocation/Physical.h>
#include <Memory.h>
#include <Multitasking/MemoryMap.h>

using namespace MemoryManagement;

x86::PageDirectory Virtual::current = 0;
x86::PageDirectory Virtual::kernel = 0;

Virtual::Virtual()
{
}

Virtual::~Virtual()
{
}

//The kernel heap is mapped into every address space
void Virtual::Initialise()
{
	unsigned int *properKernel = (unsigned int *)PageDirectoryLocation;
	//This is a higher-half kernel which uses paging, so I've already got a prefilled page directory.
	//I just need to make a note of the location and set up the recursive mapping
	asm volatile ("mov %%cr3, %0" : "=r"(kernel));
	current = kernel;
	/*
	* This is effectively a waste of 1 MiB of memory. Unfortunately, it's necessary because the heaps have to stay
	* consistent across every address space, so that the kernel can access the necessary allocations. The only way
	* that I know of doing this is to link the page tables, which requires me to allocate them all.
	* There's a small consolation prize though - I only have to allocate up to HeapEnd, which saves a few KiB
	*
	* The mapping at 0xFFF40000 may look odd, but it's due to the recursive page mapping. It doesn't actually occupy any
	* of the page directory entries; Bochs just lays out the address space in a weird way
	*/
	for(unsigned int i = KernelHeapStart >> 22; i <= (KernelHeapEnd >> 22); i++)
	{
		if((properKernel[i] & x86::PageDirectoryFlags::Present) == 0)
			properKernel[i] = (unsigned int)Physical::PageAllocator::AllocatePage(false) |
				x86::PageDirectoryFlags::Present | x86::PageDirectoryFlags::ReadWrite;
		else
			asm volatile ("cli;hlt" : : "a"(0xBAD));
	}
}

x86::PageDirectory Virtual::GetPageDirectory()
{
	return current;
}

x86::PageDirectory Virtual::GetKernelDirectory()
{
	return kernel;
}

//This method clones a page directory. If I wanted fork() support, then I could use something apart from the kernel directory
x86::PageDirectory Virtual::ClonePageDirectory(x86::PageDirectory dir)
{
	/*
	* First of all, I need to allocate 0x1000 bytes to act as a page directory
	* Then iterate from 0 to 1024. If i is above 768, then the address is above 0xC0000000 and the page table gets linked
	* Finally, set up the recursive mapping into the last page directory entry and so on
	* I've added indentation for clarity - multiple uses of temporary mappings can prove tricky
	*/

	//Allocate a physical page for the page directory.
	unsigned int pageDirectoryPhys = (unsigned int)Physical::PageAllocator::AllocatePage(false);
	unsigned int *pageDirectoryVirt = (unsigned int *)0xFFBFD000;
	//The kernel boundary is the page directory entry index of the 3.75 GiB mark
	unsigned int kernelBoundary = 0xF0000000 >> 22;
	unsigned int *cloneDirectory = (unsigned int *)dir;

	//Map the page directory into the address space so it can be accessed
	//This address is the last address guaranteed to be free
	MapMemory((unsigned int)pageDirectoryPhys, (unsigned int)pageDirectoryVirt, x86::PageDirectoryFlags::ReadWrite);
	//dir is a physical address, and so needs a temporary mapping to access
	MapMemory((unsigned int)cloneDirectory, 0xFFBFC000, x86::PageDirectoryFlags::ReadWrite);
	cloneDirectory = (unsigned int *)0xFFBFC000;

		//I subtract one because there's no point setting the final page directory entry twice
		for(unsigned int i = 0; i < 1024 - 1; i++)
		{
			//This relies on the fact that the temporary mappings I create are sequential in memory
			if((i >= (0xFFBFC000 >> 22)) && (i <= (0xFFBFF000 >> 22)))
				continue;
			if(i >= kernelBoundary)
				pageDirectoryVirt[i] = cloneDirectory[i];
			if((i >= (KernelHeapStart >> 22)) && (i <= (KernelHeapEnd >> 22)))
				pageDirectoryVirt[i] = cloneDirectory[i];
			/*
			* I could keep working on this if I wanted to clone the page tables which address the bottom 3 GiB.
			* I don't really see the need. It's not going to do much for the process, as everything below 3 GiB shouldn't
			* go into a new page directory anyway. If I wanted fork() support, then I could add the page table cloning
			*/
		}

		//The last page directory entry points to the page directory itself
		pageDirectoryVirt[1023] = pageDirectoryPhys | x86::PageDirectoryFlags::Present | x86::PageDirectoryFlags::ReadWrite;

	//Get rid of the temporary mappings
	EraseMapping((void *)pageDirectoryVirt);
	//Remove the mapping of the original directory
	EraseMapping((void *)cloneDirectory);
	return (x86::PageDirectory)pageDirectoryPhys;
}

//dir must be a physical address
void Virtual::SwitchPageDirectory(x86::PageDirectory dir)
{
	current = dir;
	//This will flush all TLB caches
	asm volatile ("mov %0, %%cr3" : : "r"(dir));
}

//src is a physical address. dest is a virtual address. flags controls things like present, COW, ring0
void Virtual::MapMemory(unsigned int src, unsigned int dest, unsigned int flags)
{
	src &= PageMask;	//Page-align src, because it's just easier to do now
	dest &= PageMask;	//dest should already be page-aligned, but lets make certain
	flags &= 0xFFF;		//flags will be the bottom bit of our page directory entry, so it can't overwrite anything

	//The recursive page mapping means that the page directory location is fixed
	unsigned int *pageDirectories = (unsigned int *)PageDirectoryLocation;
	//The directory contains a list of structures which map virtual addresses to physical
	unsigned int directoryIndex = dest >> 22;
	//Shift the address right 12 bits and mask out the directory index to get the page table index
	unsigned int tableIndex = dest >> 12;
	//Contains the page tables
	unsigned int *pageTables = (unsigned int *)PageTableLocation;

	//To save space, we don't allocate a page for every page directory. Otherwise, we would use (4 KiB * 1024) = 4 MiB
	//A zeroed page table means that we've not used this 4 MiB region for a while
	if(pageDirectories[directoryIndex] == 0)
	{
		void *pg = Physical::PageAllocator::AllocatePage();

		if(pg == 0)
			asm volatile ("cli; hlt" : : "a"(0xDEAD), "b"(0xC0DE));
		/*
		* Note that I set the user-accessible flag. This is perfectly safe - the Intel manuals state that a user process
		* can only access an address if every paging structure has the user-accessible flag set. The page tables have the control here
		*/
		pageDirectories[directoryIndex] = (unsigned int)pg | x86::PageDirectoryFlags::Present | x86::PageDirectoryFlags::ReadWrite |
			x86::PageDirectoryFlags::UserAccessible;

		//All the page tables are mapped sequentially in memory
	}
	pageTables[tableIndex] = src | flags | x86::PageDirectoryFlags::Present;
}

void Virtual::MapMemory(unsigned int src, unsigned int flags)
{
	MapMemory(src, src, flags);
}

void Virtual::EraseMapping(void *mem)
{
	mem = (void *)((unsigned int)mem & PageMask);
	//Recursive page mapping means that the page directories are at a fixed location
	unsigned int *pageDirectories = (unsigned int *)PageDirectoryLocation;
	unsigned int directoryIndex = (unsigned int)mem >> 22;
	//Ditto with page tables
	unsigned int *pageTables = (unsigned int *)PageTableLocation;
	unsigned int tableIndex = (unsigned int)mem >> 12;

	pageTables[tableIndex] = 0;
	asm volatile ("invlpg (%0)" : : "a"(mem));
	//This little bit of logic just enables the OS to free a page directory if there aren't any mappings in it
	for(unsigned int i = 0; i < 1024; i++)
	{
		//The page tables are laid out sequentially in memory; this makes things easier to work with
		//I can't directly access the page directory because it only contains physical addresses
		if(pageTables[(directoryIndex << 10) | i] != 0)
			return;
	}
	//Mark the page that the page directory entry in as free and remove it from the page tables
	Physical::PageAllocator::FreePage((void *)(pageDirectories[directoryIndex] & PageMask));
	pageDirectories[directoryIndex] = 0;
}

//mem is a virtual address
unsigned int Virtual::RetrieveMapping(void *mem, unsigned int *flags)
{
	//Bits 22:31 of a virtual address are the page directory index
	unsigned int directoryIndex = ((unsigned int)mem >> 22);
	//The middle 10 bits of a virtual address are the page table index
	unsigned int tableIndex = (unsigned int)mem >> 12;
	//The page directories are always mapped to PageDirectoryLocation
	unsigned int *pageDirectories = (unsigned int *)PageDirectoryLocation;
	unsigned int *pageTables = (unsigned int *)PageTableLocation;
	//The bottom 12 bits are the offset within a page
	unsigned int offset = (unsigned int)mem & 0xFFF;

	if((pageDirectories[directoryIndex] != 0) && (pageTables[tableIndex] != 0))
	{
		if(flags != 0)
			*flags = pageTables[tableIndex] & ~PageMask;
		return (pageTables[tableIndex] & PageMask) | offset;
	}
	else
	{
		if(flags != 0)
			*flags = 0;
		return 0;
	}
}

/*
* source is a valid virtual address. destination is not, and so will require a mapping to the underlying physical address to be created
* in the current virtual address space
*/
void Virtual::CopyToAddressSpace(unsigned int source, unsigned int length, unsigned int destination, x86::PageDirectory addressSpace,
	bool addressMustExist, bool userMode)
{
	//Page directory index of the destination page
	unsigned int destinationDirectoryIndex = destination >> 22;
	//See above, but page table index
	unsigned int destinationTableIndex = (destination >> 12) & 0x3FF;
	//This is the virtual address of addressSpace
	unsigned int *pageDirVirt = (unsigned int *)0xFFBFD000;
	//When I access the new virtual address, I will want an offset to work with
	unsigned int offset = 0;

	//Map the foreign page directory into the host address space as a fixed addrss
	MapMemory(addressSpace, (unsigned int)pageDirVirt, x86::PageDirectoryFlags::ReadWrite);
		for(unsigned int i = source & PageMask; i <= ((source + length) & PageMask); i += PageSize)
		{
			//bytesToCopy is the total number of bytes in the memory block, subtract the amount that's actually been transferred
			//If the remainder is above PageSize, then I should cap it for the sake of efficiency
			unsigned int bytesToCopy = length - (i - (source & PageMask));
			unsigned int adjustedDestination = i - (source & PageMask) + destination;
			unsigned int physicalAddress;
			//This is the actual page directory entry
			unsigned int *pageTable;
			unsigned int *page;

			offset = i - (source & PageMask);
			if(bytesToCopy > PageSize)
				bytesToCopy = PageSize;
			destinationDirectoryIndex = adjustedDestination >> 22;
			destinationTableIndex = (adjustedDestination >> 12) & 0x3FF;
			pageTable = (unsigned int *)pageDirVirt[destinationDirectoryIndex];
			if(((unsigned int)pageTable & x86::PageDirectoryFlags::Present) != x86::PageDirectoryFlags::Present)
			{
				//If the address must be present in the destination virtual address space, then there's something wrong. Die quickly
				if(addressMustExist)
					break;
				//Every page directory entry is user accessible. Page table entries however, aren't
				pageDirVirt[destinationDirectoryIndex] = (unsigned int)Physical::PageAllocator::AllocatePage(false) |
					x86::PageDirectoryFlags::Present | x86::PageDirectoryFlags::ReadWrite | x86::PageDirectoryFlags::UserAccessible;
				pageTable = (unsigned int *)pageDirVirt[destinationDirectoryIndex];
			}
			//Remove the flags to get a consistent address
			pageTable = (unsigned int *)((unsigned int)pageTable & PageMask);
			//By now, the page table is definitely present. Proceed to examine the necessary page table entry
			MapMemory((unsigned int)pageTable, 0xFFBFE000, x86::PageDirectoryFlags::ReadWrite);
				pageTable = (unsigned int *)0xFFBFE000;
				//Get the right page table entry from the index
				page = (unsigned int *)pageTable[destinationTableIndex];
				if(((unsigned int)page & x86::PageDirectoryFlags::Present) != x86::PageDirectoryFlags::Present)
				{
					unsigned int flags = x86::PageDirectoryFlags::Present | x86::PageDirectoryFlags::ReadWrite;
					//If it isn't present when I expect it to be, clean up and run
					if(addressMustExist)
					{
						EraseMapping((void *)pageTable);
						break;
					}
					//Passing user-mode indicates that the memory should not just be accessible by the kernel
					//The parameter, along with [addressMustExist], simplifies matter by using the same method for process creation and IPC
					if(userMode)
						flags |= x86::PageDirectoryFlags::UserAccessible;
					pageTable[destinationTableIndex] = (unsigned int)Physical::PageAllocator::AllocatePage(false) | flags;
					page = (unsigned int *)pageTable[destinationTableIndex];
				}

				//At the moment, the offset within the page doesn't really matter. It'll be page-aligned anyway
				physicalAddress = (unsigned int)page & PageMask;
				//I have the physical address of the place I need to map it to. Now, just map it in and copy away
				MapMemory(physicalAddress, 0xFFBFF000, x86::PageDirectoryFlags::ReadWrite);
					Memory::Copy((void *)(0xFFBFF000 + offset), (const void *)source, bytesToCopy);
				EraseMapping((void *)0xFFBFF000);
			EraseMapping((void *)pageTable);
		}
	EraseMapping((void *)pageDirVirt);
}
