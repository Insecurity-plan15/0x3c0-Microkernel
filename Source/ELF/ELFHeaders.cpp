#include <ELF/ELFHeaders.h>
#include <Common/Strings.h>

using namespace ELF;
using namespace ELF::FileStructures;

ELFObject::ELFObject()
{
	valid = false;
	linkedTo = new LinkedList<ELFObject *>(0);
	linkedAgainst = new LinkedList<ELFObject *>(0);
	dependancies = new LinkedList<char *>(0);
}

ELFObject::~ELFObject()
{
}

unsigned int ELFObject::hashSymbol(const unsigned char *name)
{
	unsigned int h = 0, g = 0;

	while(*name)
	{
		h = (h << 4) + *name++;
		if((g = (h & 0xF0000000)) != 0)	//The operator precendence is implicit in the ELF ABI
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

unsigned int ELFObject::getBucketElement(unsigned int idx)
{
	//(idx + 2): We need to skip the nChain and nBucket elements
	unsigned int *ptr = (unsigned int *)&hashTable[idx + 2];

	return *ptr;
}

unsigned int ELFObject::getChainElement(unsigned int idx)
{
	//The chain elements follow the bucket, so we skip the bucket list and the nChain and nBucket elements
	unsigned int *ptr = (unsigned int *)&hashTable[idx + nBucket + 2];

	return *ptr;
}

Symbol *ELFObject::getHashedSymbol(const unsigned char *name)
{
	unsigned int hash = hashSymbol(name);
	unsigned int chainStart = hash % nBucket;

	for(unsigned int index = getBucketElement(chainStart); index != 0; index = getChainElement(index))
	{
		Symbol *symbol = &dynamicSymbolTable[index];

		if(symbol->Name == 0)
			continue;
		if(Strings::Compare((char *)name, dynamicStringTable + symbol->Name) == 0)
			return symbol;
	}
	return 0;
}

void ELFObject::parseDynamicHeader(ProgramHeader *dynamicHeader)
{
	if(dynamicHeader && dynamicHeader->Type == 2)
	{
		DynamicSymbol *dynamicSymbol = (DynamicSymbol *)((unsigned int)header + dynamicHeader->Offset);

		dynamicSection = dynamicSymbol;
		while(dynamicSymbol->Tag != 0)	//DT_NULL
		{
			switch(dynamicSymbol->Tag)
			{
			case 1:	//DT_NEEDED
				dependancies->Add((char *)(dynamicSymbol->Union.Pointer + base));
				break;
			case 2:	//DT_PLTRELSZ
				pltRelocationTableSize = dynamicSymbol->Union.Value;
				break;
			case 3:	//DT_PLTGOT
				globalOffsetTable = (unsigned int *)(dynamicSymbol->Union.Pointer + base);
				break;
			case 4:	//DT_HASH
				hashTable = (unsigned int *)(dynamicSymbol->Union.Pointer + base);
				break;
			case 5:	//DT_STRTAB
				dynamicStringTable = (char *)(dynamicSymbol->Union.Pointer + base);
				break;
			case 6:	//DT_SYMTAB
				dynamicSymbolTable = (Symbol *)(dynamicSymbol->Union.Pointer + base);
				break;
			case 7:	//DT_RELA
				relocationATable = (RelocationEntryA *)(dynamicSymbol->Union.Pointer + base);
				break;
			case 8:	//DT_RELASZ
				relocationATableSize = dynamicSymbol->Union.Value;
				break;
			case 10://DT_STRSZ
				dynamicStringTableSize = dynamicSymbol->Union.Value;
				break;
			case 14://DT_SONAME
				name = (char *)(dynamicSymbol->Union.Value + base);
				break;
			case 17://DT_REL
				relocationTable = (RelocationEntry *)(dynamicSymbol->Union.Pointer + base);
				break;
			case 18://DT_RELSZ
				relocationTableSize = dynamicSymbol->Union.Value;
				break;
			case 20://DT_PLTREL
				relocationUsesAddends = (dynamicSymbol->Union.Value == 7);	//DT_RELA
				break;
			case 23://DT_JMPREL
				if(relocationUsesAddends)
					pltRelocationATable = (RelocationEntryA *)(dynamicSymbol->Union.Pointer + base);
				else
					pltRelocationTable = (RelocationEntry *)(dynamicSymbol->Union.Pointer + base);
				break;
			}
			dynamicSymbol++;
		}
	}
}

bool ELFObject::IsValid()
{
	return valid;
}

unsigned int ELFObject::GetBase()
{
	return base;
}

unsigned int ELFObject::GetEnd()
{
	return elfEnd;
}

Symbol *ELFObject::GetSymbolTable()
{
	return symbolTable;
}

Symbol *ELFObject::GetDynamicSymbolTable()
{
	return dynamicSymbolTable;
}

char *ELFObject::GetStringTable()
{
	return stringTable;
}

char *ELFObject::GetDynamicStringTable()
{
	return dynamicStringTable;
}

char *ELFObject::GetSymbol(unsigned int address, bool inexact, bool useDynamic)
{
	Symbol *symbols = useDynamic ? GetDynamicSymbolTable() : GetSymbolTable();
	char *strings = useDynamic ? GetDynamicStringTable() : GetStringTable();
	unsigned int symbolsSize = useDynamic ? dynamicSymbolTableSize : symbolTableSize;
	Symbol *fallback = 0;
	unsigned int max = 0;

	if(symbols == 0)	//If the proper symbol table can't be found, return 0
		return 0;
	if(strings == 0)	//Same as the above, but with the string table
		return 0;
	for(unsigned int i = 0; i < symbolsSize / sizeof(Symbol); i++)
	{
		Symbol *sym = &symbols[i];
		char *name = &strings[sym->Name];

		if((address >= sym->Value) && (address < sym->Value + sym->Size))
			return name;
		if(inexact)
		{
			if(sym->Value > max && sym->Value <= address)
			{
				max = sym->Value;
				fallback = sym;
			}
		}
	}
	if(inexact && fallback != 0)
	{
		char *name = &strings[fallback->Name];

		return name;
	}
	return 0;
}

Symbol *ELFObject::GetSymbol(char *symbolName, bool useDynamic)
{
	Symbol *match = 0;

	if(useDynamic)
	{
		//If the hash table is available, use that. Otherwise, we can't do anything
		if(hashTable != 0)
			return getHashedSymbol((const unsigned char *)symbolName);
		else
			return 0;	//We return 0 because the only way we know how many dynamic symbols to look through is by the hash table
	}
	else
	{
		for(unsigned int i = 0; i < symbolTableSize / sizeof(Symbol); i++)
		{
			match = &symbolTable[i];
			char *matchName = &stringTable[match->Value];

			if(Strings::Compare(symbolName, matchName) == 0)
				return match;
		}
	}
	return match;
}

unsigned int ELFObject::GetAddress(char *symbolName, bool useDynamic)
{
	Symbol *match = GetSymbol(symbolName, useDynamic);

	if(match != 0)
		return match->Value;
	else
		return 0;
}

LinkedList<ELFObject *> *ELFObject::LinkedTo()
{
	return linkedTo;
}

LinkedList<ELFObject *> *ELFObject::LinkedAgainst()
{
	return linkedAgainst;
}

LinkedList<char *> *ELFObject::Dependancies()
{
	return dependancies;
}

char *ELFObject::GetName()
{
	return name;
}

unsigned int ELFObject::GetType()
{
	return type;
}

unsigned int *ELFObject::GetGOT()
{
	return globalOffsetTable;
}
