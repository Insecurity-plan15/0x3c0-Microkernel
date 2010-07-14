#include <SystemCalls/POSIXInterface.h>

using namespace SystemCalls::POSIX;

Internal::Internal()
{
}

Internal::~Internal()
{
}

SystemCallDefinition(fork)
{
    //Clones a process' address space
    //Only the calling thread is cloned
    //All file handles remain open

    //to avoid having to copy and rebase the stack (horrid idea), I'll simply put it in the same memory address as the parent
    //fork() will already have copied the values themselves, making this the simplest way
    return 0;
}

SystemCallDefinition(execve)
{
    //Receives a collection of memory images
    //All address space outside of heaps is wiped, and memory image loaded
    //Current threads deleted, new thread added
    //  initial values of environment, arguments, EIP, etc

    //mental note: must alter return EIP, otherwise I'll page fault
    return 0;
}

SystemCallInterrupt::SystemCallInterrupt()
    : InterruptSink(0x40)
{
    calls[0] = Internal::fork;
    calls[1] = Internal::execve;
}

SystemCallInterrupt::~SystemCallInterrupt()
{
}

void SystemCallInterrupt::receive(StackStates::Interrupt *stack)
{
    unsigned int eip = stack->EIP;
	unsigned int eax = 0;

	if(stack->EDX >= (sizeof(calls) / sizeof(SystemCall)))
	{
		//Somebody's trying to mess up the kernel. Stop them
		stack->EAX = 0xFFFFFFFF;
		return;
	}
	SystemCall sc = calls[stack->EDX];

	/*
	* This takes into account the fact that the execve system call can change the values in the stack. If I change the new stack state,
	* it'll be inconsistent, and in this case the return address will go wobbly.
	*/
	eax = (*sc)(stack->EAX, stack->EBX, stack->ECX, stack);
	if(stack->EIP == eip)
		stack->EAX = eax;
}
