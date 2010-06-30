#include <Common/CPlusPlus.h>
#include <MemoryAllocation/AllocateMemory.h>
#include <Memory.h>

extern "C"
{
	void invokeConstructors()
	{
		extern unsigned int constructorStart;
		extern unsigned int constructorEnd;

		for(unsigned int *ctor = &constructorStart; ctor < &constructorEnd; ctor++)
			((void (*)())*ctor)();
	}

	void __cxa_pure_virtual()
	{
		//If a virtual method call cannot be made, this function is invoked.
	}
}

void *operator new(long unsigned int sz) throw()
{
	MemoryManagement::Heap *hp = MemoryManagement::Heap::GetKernelHeap();

	if(hp != 0)
		return hp->Allocate((unsigned int)sz);
	else
		return 0;
}

void *operator new(long unsigned int sz, unsigned int placement) throw()
{
	return (void *)placement;
}

void *operator new(long unsigned int sz, Process *&p) throw()
{
	return p->GetAllocator()->Allocate(sz);
}

void *operator new[](long unsigned int sz) throw()
{
	MemoryManagement::Heap *hp = MemoryManagement::Heap::GetKernelHeap();

	if(hp != 0)
		return hp->Allocate((unsigned int)sz);
	else
		return 0;
}

void *operator new[](long unsigned int sz, unsigned int placement) throw()
{
	return (void *)placement;
}

void *operator new[](long unsigned int sz, Process *&p) throw()
{
	return p->GetAllocator()->Allocate(sz);
}

void operator delete(void *p)
{
	MemoryManagement::Heap *hp = MemoryManagement::Heap::GetKernelHeap();

	if(hp != 0)
		hp->Free(p);
}

void operator delete(void *p, Process *&process)
{
	return process->GetAllocator()->Free(p);
}

void operator delete[](void *p)
{
	MemoryManagement::Heap *hp = MemoryManagement::Heap::GetKernelHeap();

	if(hp != 0)
		hp->Free(p);
}

void operator delete[](void *p, Process *&process)
{
	return process->GetAllocator()->Free(p);
}
