#ifndef ELFHeaders_H
#define ELFHeaders_H

#include <LinkedList.h>
#include <Multitasking/Process.h>

#define GetRelocationType(i) ((i) & 0xF)
#define GetRelocationSymbol(i) ((i) >> 8)

#define GetSymbolType(i) GetRelocationType(i)
#define GetSymbolBinding(i) ((i) >> 4)

namespace ELF
{
	namespace FileStructures
	{
		struct ELFHeader
		{
			unsigned char Checksum[16];			//Identifies the file as an ELF object
			unsigned short Type;				//Depending upon the type, I could be processing a shared object, executable or other object
			unsigned short Machine;				//e_machine
			unsigned int Version;				//e_version
			unsigned int EntryPoint;			//Location of the entry point
			unsigned int ProgramHeaderOffset;	//Start of a sequential array of program headers
			unsigned int SectionHeaderOffset;	//Same as above, with section headers
			unsigned int Flags;					//ELF file flags
			unsigned short ELFHeaderSize;		//Should be equal to sizeof(ELFHeader)
			unsigned short ProgramHeaderSize;	//Should be equal to sizeof(ProgramHeader)
			unsigned short ProgramHeaderCount;	//Number of program headers
			unsigned short SectionHeaderSize;	//Should be equal to sizeof(SectionHeader)
			unsigned short SectionHeaderCount;	//Number of section headers
			unsigned short SectionHeaderStringTableIndex;	//This is an index into the section header string table, which contains the names of the section headers
		} __attribute__((packed));

		struct SectionHeader
		{
			unsigned int Name;		//String table index
			unsigned int Type;		//Header type
			unsigned int Flags;		//Flags
			unsigned int Address;	//Address
			unsigned int Offset;	//Offset in file
			unsigned int Size;		//Size
			unsigned int Link;		//Link to other section
			unsigned int Info;		//Pointer to info
			unsigned int Alignment;	//Address alignment
			unsigned int EntrySize;	//Size of entries in this section
		} __attribute__((packed));
	}

	class ELFObject
	{
	protected:
		FileStructures::ELFHeader *header;

		bool valid;

		unsigned int base;
		unsigned int elfEnd;

		unsigned int type;

		Process *process;
	public:
		ELFObject();
		virtual ~ELFObject();
		bool IsValid();

		unsigned int GetBase();
		unsigned int GetEnd();

		virtual FileStructures::SectionHeader *GetSectionHeader(unsigned int idx) = 0;
		virtual void Start(void *parameter) = 0;
	};
}
#endif
