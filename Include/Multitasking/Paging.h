#ifndef Paging_H
#define Paging_H

#include <Typedefs.h>

#define PageMask				0xFFFFF000
#define PageSize				0x1000
#define KernelVirtualAddress	0xFFFF800000000000

#define PageDirectoryLocation	0xFFFFF000
#define PageTableLocation		0xFFC00000

namespace MemoryManagement
{
	namespace x86
	{
		namespace PageDirectoryFlags
		{
			//Standard Intel bits follow
			const unsigned int Present = 0x1;		//If page isn't present, a page fault occurs
			const unsigned int ReadWrite = 0x2;		//Can guard specific 4 MiB pages
			const unsigned int UserAccessible = 0x4;//Used to prevent user-mode from accessing the page. 0 prevents access
			const unsigned int WriteThrough = 0x8;
			const unsigned int CacheDisable = 0x10;	//Commonly used for memory-mapped devices (APICs, PCI, HPET)
			const unsigned int Accessed = 0x20;		//Set by the processor when the page is used to look up an address
			const unsigned int Dirty = 0x40;		//Set by the processor when the page is written to
			const unsigned int PageSizeBit = 0x80;	//Must be 1 to avoid referencing a page table
			const unsigned int Global = 0x100;		//If this bit is set, the page is kept in the TLB buffer
			//My special meanings for bits go here
			const unsigned int CopyOnWrite = 0x200;
		}
		//Various paging stuff goes here
		typedef physAddress PageDirectory;
	}

	class Virtual
	{
	private:
		static x86::PageDirectory current;
		static x86::PageDirectory kernel;
		static void clonePageTable(unsigned int *pageDirectory, unsigned int idx);
		Virtual();
		~Virtual();
	public:
		static void Initialise();

		static x86::PageDirectory GetPageDirectory();
		static x86::PageDirectory GetKernelDirectory();
		static x86::PageDirectory ClonePageDirectory(x86::PageDirectory dir = kernel, bool usePosixSemantics = false);
		static void SwitchPageDirectory(x86::PageDirectory dir);

		static void MapMemory(physAddress src, virtAddress dest, uint64 flags);
		static void MapMemory(physAddress src, uint64 flags);
		static void EraseMapping(virtAddress mem);
		static physAddress RetrieveMapping(virtAddress mem, uint64 *flags);

		//This method will copy [length] bytes from [source] in this addreses space to [destination] in the [addressSpace] address space
		//It's used by message passing and when creating a new process from a memory range
		static void CopyToAddressSpace(virtAddress source, unsigned int length, virtAddress destination, x86::PageDirectory addressSpace,
			bool addressMustExist = true, bool userMode = true);
	};
}
#endif
