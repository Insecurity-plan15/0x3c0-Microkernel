#ifndef CPlusPlus_H
#define CPlusPlus_H

#include <Multitasking/Process.h>

extern "C" void invokeConstructors();

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
