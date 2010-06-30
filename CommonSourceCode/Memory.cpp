#include <Memory.h>

Memory::Memory()
{
}

Memory::~Memory()
{
}

void Memory::Clear(void *src, unsigned int size)
{
	for(unsigned int i = 0; i < size; i++)
		((char *)src)[i] = 0;
}

void Memory::Copy(void *destination, const void *src, unsigned int num)
{
	for(unsigned int i = 0; i < num; i++)
		((char *)destination)[i] = ((char *)src)[i];
}
