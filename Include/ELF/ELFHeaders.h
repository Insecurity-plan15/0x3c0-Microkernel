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

		struct ProgramHeader
		{
			//The only important Type is 1: PT_LOAD
			unsigned int Type;
			unsigned int Offset;
			//We're using identity mapping, so VirtualAddress and PhysicalAddress must be identical
			unsigned int VirtualAddress;
			unsigned int PhysicalAddress;
			unsigned int FileSize;	//FileSize can never be greater than MemorySize
			//Any difference between FileSize and MemorySize is filled with zeros
			unsigned int MemorySize;
			unsigned int Flags;
			unsigned int Alignment;
		} __attribute__((packed));

		struct DynamicSymbol
		{
			//Used to differentiate between tags
			int Tag;
			union
			{
				//Depending on the value of Tag, either of these could be valid
				int Value;
				unsigned int Pointer;
			} Union;
		} __attribute__((packed));

		struct RelocationEntry
		{
			//This is the only type of relocation entry used in x86 file formats
			unsigned int Offset;
			unsigned int Information;
		} __attribute__((packed));

		struct RelocationEntryA
		{
			unsigned int Offset;
			unsigned int Information;
			unsigned int Addend;	//Also known as size
		} __attribute__((packed));

		struct Symbol
		{
			unsigned int Name;	//Index into one of the string tables
			unsigned int Value;	//Address of the symbol
			unsigned int Size;
			unsigned char Info;	//Contains various data
			unsigned char Reserved;
			unsigned short SectionHeaderIndex;	//If this is 0, the symbol value is located in another file
		} __attribute__((packed));
	}

	class ELFObject
	{
	protected:
		unsigned int *buffer;
		FileStructures::ELFHeader *header;

		bool valid;
		bool relocationUsesAddends;

		unsigned int base;
		unsigned int elfEnd;

		FileStructures::Symbol *symbolTable;
		unsigned int symbolTableSize;

		char *stringTable;
		unsigned int stringTableSize;

		FileStructures::Symbol *dynamicSymbolTable;
		unsigned int dynamicSymbolTableSize;

		char *dynamicStringTable;
		unsigned int dynamicStringTableSize;

		LinkedList<ELFObject *> *linkedTo;
		LinkedList<ELFObject *> *linkedAgainst;
		unsigned int *globalOffsetTable;
		FileStructures::DynamicSymbol *dynamicSection;
		LinkedList<char *> *dependancies;

		FileStructures::RelocationEntry *pltRelocationTable;
		FileStructures::RelocationEntryA *pltRelocationATable;
		FileStructures::SectionHeader *pltRelocationHeader;
		unsigned int pltRelocationTableSize;

		FileStructures::RelocationEntryA *relocationATable;
		unsigned int relocationATableSize;
		FileStructures::RelocationEntry *relocationTable;
		unsigned int relocationTableSize;

		unsigned int *hashTable;
		unsigned int nBucket;
		unsigned int nChain;
		unsigned int *bucketStart;
		unsigned int *chainStart;

		char *name;
		unsigned int type;

		Process *process;

		unsigned int hashSymbol(const unsigned char *name);
		unsigned int getBucketElement(unsigned int idx);
		unsigned int getChainElement(unsigned int idx);
		FileStructures::Symbol *getHashedSymbol(const unsigned char *name);
		void parseDynamicHeader(FileStructures::ProgramHeader *dynamicHeader);
	public:
		ELFObject();
		virtual ~ELFObject();
		bool IsValid();

		unsigned int GetBase();
		unsigned int GetEnd();

		FileStructures::Symbol *GetSymbolTable();
		FileStructures::Symbol *GetDynamicSymbolTable();

		char *GetStringTable();
		char *GetDynamicStringTable();

		char *GetSymbol(unsigned int address, bool inexact, bool useDynamic);
		FileStructures::Symbol *GetSymbol(char *symbolName, bool useDynamic);
		unsigned int GetAddress(char *symbolName, bool useDynamic);

		virtual FileStructures::SectionHeader *GetSectionHeader(unsigned int idx) = 0;
		virtual FileStructures::ProgramHeader *GetProgramHeader(unsigned int idx) = 0;

		LinkedList<ELFObject *> *LinkedTo();
		LinkedList<ELFObject *> *LinkedAgainst();
		LinkedList<char *> *Dependancies();

		char *GetName();
		unsigned int GetType();

		unsigned int *GetGOT();
		virtual unsigned int ResolveSymbol(unsigned int address) = 0;

		virtual void Start(void *parameter) = 0;
	};
}
#endif
