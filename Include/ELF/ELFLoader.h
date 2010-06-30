#ifndef ELFLoader_H
#define ELFLoader_H

#include <ELF/ELFHeaders.h>

namespace ELF
{
	class ELFLoader : public ELFObject
	{
	friend class Process;
	public:
		ELFLoader(void *base, unsigned int sz, char *defaultName);
		~ELFLoader();
		FileStructures::SectionHeader *GetSectionHeader(unsigned int idx);
		FileStructures::ProgramHeader *GetProgramHeader(unsigned int idx);
		unsigned int ResolveSymbol(unsigned int address);
		void Start(void *parameter);
		Process *GetProcess();
		void SetProcess(Process *p);
	};
}

#endif
