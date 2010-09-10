#include <ELF/ELFLoader.h>
#include <Common/Strings.h>
#include <Common/CPlusPlus.h>
#include <Multitasking/Paging.h>
#include <MemoryAllocation/Physical.h>
#include <Memory.h>

using namespace ELF;
using namespace ELF::FileStructures;

ELFLoader::ELFLoader(virtAddress base, unsigned int sz)
{
	elfEnd = (virtAddress)(base + sz);
	valid = true;
	header = (ELFHeader *)base;
	type = header->Type;
	if(header->Checksum[0] != 0x7F ||
			header->Checksum[1] != 'E' ||
			header->Checksum[2] != 'L' ||
			header->Checksum[3] != 'F')
		valid = false;
	//Types 1 and 2 are relocatable and executable files respectively
	if(type == 0 || type >= 3)
		valid = false;
	if(header->Checksum[4] != 1)	//32-bit code
		valid = false;
	if(header->Checksum[5] != 1)	//Least significant bit. I only load little-endian programs
		valid = false;

	if(valid == false)
		return;
}

ELFLoader::~ELFLoader()
{
	process->SetELFObject(0);
}

SectionHeader *ELFLoader::GetSectionHeader(unsigned int idx)
{
	if(idx > header->SectionHeaderCount)
		return 0;
	return (SectionHeader *)((virtAddress)header + header->SectionHeaderOffset + idx * header->SectionHeaderSize);
}

void ELFLoader::Start(void *parameter)
{
	unsigned int cr3 = MemoryManagement::Virtual::GetPageDirectory();
	Thread *thread;
	void **params = 0;

	//If there's no process to load it into, it doesn't matter
	if(process == 0)
		return;
	thread = new Thread((virtAddress)header->EntryPoint, process);
	//The first point of order is to switch address spaces, so the right process gets the data
	MemoryManagement::Virtual::SwitchPageDirectory(process->GetPageDirectory());
	params = new (process) void *[1];
	params[0] = parameter;
	for(unsigned int i = 0; i < header->SectionHeaderCount; i++)
	{
		SectionHeader *sh = GetSectionHeader(i);
		virtAddress address = (virtAddress)(sh->Address & PageMask);
		unsigned int offset = sh->Offset;
		//This might mean allocating slightly more memory than I should, but I can't think of any other way
		virtAddress endAddress = (virtAddress)(((sh->Address + sh->Size) & PageMask) + PageSize);

		//There's a corner case with endAddress: if it's already page-aligned, then I'm not going to allocate extra
		if( ((sh->Address + sh->Size) & ~PageMask) == 0)
			endAddress = (virtAddress)(sh->Address + sh->Size);
		if(sh->Size == 0 || sh->Address == 0)
			continue;
		switch (sh->Type)
		{
			//A type 1 section header contains program code. It needs mapping as present, read-only
			case 1:
			{
				//Map the pages to memory
				for(virtAddress i = address; i <= endAddress; i += PageSize)
				{
					physAddress pg = MemoryManagement::Physical::PageAllocator::AllocatePage(false);
					uint64 currentMapping[2];
					unsigned int flags = MemoryManagement::x86::PageDirectoryFlags::Present |
						MemoryManagement::x86::PageDirectoryFlags::UserAccessible;

					//If bit 1 is set, then the memory is not program code and should be mapped as read-write
//					if((sh->Flags & 0x1) == 0x1)
						flags |= MemoryManagement::x86::PageDirectoryFlags::ReadWrite;
					currentMapping[0] = MemoryManagement::Virtual::RetrieveMapping(i, &currentMapping[1]);
					//If a mapping is already in place, roll back
					if(pg == 0 || ((currentMapping[1] & MemoryManagement::x86::PageDirectoryFlags::Present) != 0))
						//Replace break with roll back
						break;
					MemoryManagement::Virtual::MapMemory((physAddress)pg, i, flags);
				}
				//Copy the data over from the position within the file to the correct place in memory
				Memory::Copy((void *)address, (void *)((virtAddress)header + offset), sh->Size);
				break;
			}
			//A type 8 section header contains variables. It must be cleared and set to 0
			case 8:
			{
				//Map the pages to memory
				for(virtAddress i = address; i <= endAddress; i += PageSize)
				{
					physAddress pg = MemoryManagement::Physical::PageAllocator::AllocatePage(false);
					uint64 currentMapping[2];
					uint64 flags = MemoryManagement::x86::PageDirectoryFlags::Present |
						MemoryManagement::x86::PageDirectoryFlags::UserAccessible;

					if((sh->Flags & 0x1) == 0x1)
						flags |= MemoryManagement::x86::PageDirectoryFlags::ReadWrite;
					currentMapping[0] = MemoryManagement::Virtual::RetrieveMapping(i, &currentMapping[1]);
					//If a mapping is already in place, roll back
					if(pg == 0 || ((currentMapping[1] & MemoryManagement::x86::PageDirectoryFlags::Present) != 0))
						//Replace break with roll back
						break;
					MemoryManagement::Virtual::MapMemory((physAddress)pg, i, flags);
				}
				//Clear the new memory, ready for variables
				Memory::Clear((void *)address, sh->Size);
				break;
			}
		}
	}
	//Finally, switch back to the original address space to revert to the previous state
	MemoryManagement::Virtual::SwitchPageDirectory(cr3);
	thread->Start(params);
}

Process *ELFLoader::GetProcess()
{
	return process;
}

void ELFLoader::SetProcess(Process *p)
{
	process = p;
	process->SetELFObject(this);
}
