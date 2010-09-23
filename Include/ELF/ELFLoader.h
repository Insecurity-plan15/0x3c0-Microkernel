#ifndef ELFLoader_H
#define ELFLoader_H

#include <ELF/ELFHeaders.h>

namespace ELF
{
	class ELFLoader : public ELFObject
	{
	friend class Process;
	public:
		ELFLoader(virtAddress bse, unsigned int sz);
		~ELFLoader();
		FileStructures::SectionHeader *GetSectionHeader(unsigned int idx);
		void Start(void *parameter);
		Process *GetProcess();
		void SetProcess(Process *p);
	};
}

#endif
