#include <Common/CPlusPlus.h>
#include <MemoryAllocation/AllocateMemory.h>
#include <Memory.h>

extern "C"
{
	void invokeConstructors()
	{
		extern virtAddress constructorStart;
		extern virtAddress constructorEnd;

		for(virtAddress *ctor = &constructorStart; ctor < &constructorEnd; ctor++)
			((void (*)())*ctor)();
	}

	//Normally, I'd store f and use it as a destructor, but I never invoke destructors so there's no point
	int __cxa_atexit(void (*f)(void *), void *objptr, void *dso)
	{
		return 0;
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

	if(hp != 0 && p != (void *)0)
		hp->Free(p);
}

void operator delete(void *p, Process *&process)
{
	return process->GetAllocator()->Free(p);
}

void operator delete[](void *p)
{
	MemoryManagement::Heap *hp = MemoryManagement::Heap::GetKernelHeap();

	if(hp != 0 && p != (void *)0)
		hp->Free(p);
}

void operator delete[](void *p, Process *&process)
{
	return process->GetAllocator()->Free(p);
}
