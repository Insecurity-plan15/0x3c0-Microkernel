#include <Memory.h>

Memory::Memory()
{
}

Memory::~Memory()
{
}

void Memory::Clear(void *src, uint64 size)
{
	for(uint64 i = 0; i < size; i++)
		((char *)src)[i] = 0;
}

void Memory::Copy(void *destination, const void *src, uint64 num)
{
	for(uint64 i = 0; i < num; i++)
		((char *)destination)[i] = ((char *)src)[i];
}
