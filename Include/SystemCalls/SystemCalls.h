#ifndef SystemCalls_H
#define SystemCalls_H

#include <Common/Interrupts.h>

typedef unsigned int (*SystemCall)(unsigned int eax, unsigned int ebx, unsigned int ecx, StackStates::Interrupt *stack);

#define SystemCallMethod(_name)	static unsigned int _name(unsigned int eax, unsigned int ebx, unsigned int ecx, StackStates::Interrupt *stack)

#define SystemCallDefinition(_name)	unsigned int Internal::_name(unsigned int eax, unsigned int ebx, unsigned int ecx, StackStates::Interrupt *stack)

#endif // SystemCalls_H
