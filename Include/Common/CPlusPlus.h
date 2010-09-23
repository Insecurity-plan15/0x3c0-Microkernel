#ifndef CPlusPlus_H
#define CPlusPlus_H

#include <Multitasking/Process.h>

extern "C"
{
	void invokeConstructors();
	void __cxa_pure_virtual();
	int __cxa_atexit(void (*f)(void *), void *objptr, void *dso);

	void *__dso_handle;
}

void *operator new(long unsigned int sz);
void *operator new(long unsigned int sz, unsigned int placement);
void *operator new(long unsigned int sz, Process *&p);
void *operator new[](long unsigned int sz);
void *operator new[](long unsigned int sz, unsigned int placement);
void *operator new[](long unsigned int sz, Process *&p);

void operator delete(void *p);
void operator delete(void *p, Process *&process);
void operator delete[](void *p);
void operator delete[](void *p, Process *&process);

#endif
