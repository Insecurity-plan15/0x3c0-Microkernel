#include <ELF/ELFLoader.h>
#include <Common/Strings.h>
#include <Multitasking/Paging.h>
#include <MemoryAllocation/Physical.h>
#include <Memory.h>

using namespace ELF;
using namespace ELF::FileStructures;

ELFLoader::ELFLoader(void *base, unsigned int sz, char *defaultName)
{
	char *sectionHeaderStringTable;
	SectionHeader *sectionHeaderStringTableHeader;

	buffer = (unsigned int *)base;
	elfEnd = (unsigned int)base + sz;
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
	sectionHeaderStringTableHeader = GetSectionHeader(header->SectionHeaderStringTableIndex);
	sectionHeaderStringTable = (char *)((unsigned int)header + sectionHeaderStringTableHeader->Offset);
	for(unsigned int i = 0; i < header->ProgramHeaderCount; i++)
	{
		ProgramHeader *ph = GetProgramHeader(i);

		if(ph != 0 && ph->Type == 2)
			parseDynamicHeader(ph);
	}
	//Not every program has an element defining its name. Allow a default one to be selected
	if(name == 0)
		name = defaultName;
	for(unsigned int i = 0; i < header->SectionHeaderCount; i++)
	{
		SectionHeader *sh = GetSectionHeader(i);
		char *sectionName = &sectionHeaderStringTable[sh->Name];

		if(sh->Size == 0)
			continue;
		switch(sh->Type)
		{
			//Every ELF file has a null section header. If this is it, then the information won't be right
			case 0:
				continue;
			//If a file has a section header with type 2, then it's a symbol table
			case 2:
			{
				if(Strings::Compare(sectionName, (char *)".symtab") == 0)
				{
					symbolTable = (Symbol *)((unsigned int)header + sh->Offset);
					symbolTableSize = sh->Size;
				}
				break;
			}
			//A type 3 section header contains a string table
			case 3:
			{
				if(Strings::Compare(sectionName, (char *)".strtab") == 0)
				{
					stringTable = (char *)((unsigned int)header + sh->Offset);
					stringTableSize = sh->Size;
				}
				break;
			}
		}
	}
	if(hashTable != 0)
	{
		nBucket = *hashTable;
		dynamicSymbolTableSize = nChain = *(hashTable + sizeof(int));
	}
}

ELFLoader::~ELFLoader()
{
	process->SetELFObject(0);
}

SectionHeader *ELFLoader::GetSectionHeader(unsigned int idx)
{
	if(idx > header->SectionHeaderCount)
		return 0;
	return (SectionHeader *)((unsigned int)header + header->SectionHeaderOffset + idx * header->SectionHeaderSize);
}

ProgramHeader *ELFLoader::GetProgramHeader(unsigned int idx)
{
	if(idx > header->ProgramHeaderCount)
		return 0;
	return (ProgramHeader *)((unsigned int)header + header->ProgramHeaderOffset + idx * header->ProgramHeaderSize);
}

unsigned int ELFLoader::ResolveSymbol(unsigned int address)
{
	return 0;
}

void ELFLoader::Start(void *parameter)
{
	unsigned int cr3 = MemoryManagement::Virtual::GetPageDirectory();
	Thread *thread;

	//If there's no process to load it into, it doesn't matter
	if(process == 0)
		return;
	thread = new Thread((ThreadStart)header->EntryPoint, process);
	//The first point of order is to switch address spaces, so the right process gets the data
	MemoryManagement::Virtual::SwitchPageDirectory(process->GetPageDirectory());
	for(unsigned int i = 0; i < header->SectionHeaderCount; i++)
	{
		SectionHeader *sh = GetSectionHeader(i);
		unsigned int address = sh->Address & PageMask;
		unsigned int offset = sh->Offset;
		//This might mean allocating slightly more memory than I should, but I can't think of any other way
		unsigned int endAddress = ((sh->Address + sh->Size) & PageMask) + PageSize;

		//There's a corner case with endAddress: if it's already page-aligned, then I'm not going to allocate extra
		if( ((sh->Address + sh->Size) & ~PageMask) == 0)
			endAddress = sh->Address + sh->Size;
		if(sh->Size == 0 || sh->Address == 0)
			continue;
		switch (sh->Type)
		{
			//A type 1 section header contains program code. It needs mapping as present, read-only
			case 1:
			{
				//Map the pages to memory
				for(unsigned int i = address; i <= endAddress; i += PageSize)
				{
					void *pg = MemoryManagement::Physical::PageAllocator::AllocatePage(false);
					unsigned int currentMapping[2];
					unsigned int flags = MemoryManagement::x86::PageDirectoryFlags::Present |
						MemoryManagement::x86::PageDirectoryFlags::UserAccessible;

					//If bit 1 is set, then the memory is not program code and should be mapped as read-write
//					if((sh->Flags & 0x1) == 0x1)
						flags |= MemoryManagement::x86::PageDirectoryFlags::ReadWrite;
					currentMapping[0] = MemoryManagement::Virtual::RetrieveMapping((void *)i, &currentMapping[1]);
					//If a mapping is already in place, roll back
					if(pg == 0 || ((currentMapping[1] & MemoryManagement::x86::PageDirectoryFlags::Present) != 0))
						//Replace break with roll back
						break;
					MemoryManagement::Virtual::MapMemory((unsigned int)pg, i, flags);
				}
				//Copy the data over from the position within the file to the correct place in memory
				Memory::Copy((void *)address, (void *)((unsigned int)header + offset), sh->Size);
				break;
			}
			//A type 8 section header contains variables. It must be cleared and set to 0
			case 8:
			{
				//Map the pages to memory
				for(unsigned int i = address; i <= endAddress; i += PageSize)
				{
					void *pg = MemoryManagement::Physical::PageAllocator::AllocatePage(false);
					unsigned int currentMapping[2];
					unsigned int flags = MemoryManagement::x86::PageDirectoryFlags::Present |
						MemoryManagement::x86::PageDirectoryFlags::UserAccessible;

					if((sh->Flags & 0x1) == 0x1)
						flags |= MemoryManagement::x86::PageDirectoryFlags::ReadWrite;
					currentMapping[0] = MemoryManagement::Virtual::RetrieveMapping((void *)i, &currentMapping[1]);
					//If a mapping is already in place, roll back
					if(pg == 0 || ((currentMapping[1] & MemoryManagement::x86::PageDirectoryFlags::Present) != 0))
						//Replace break with roll back
						break;
					MemoryManagement::Virtual::MapMemory((unsigned int)pg, i, flags);
				}
				//Clear the new memory, ready for variables
				Memory::Clear((void *)address, sh->Size);
				break;
			}
		}
	}
	//Finally, switch back to the original address space to revert to the previous state
	MemoryManagement::Virtual::SwitchPageDirectory(cr3);
	thread->Start(parameter);
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
