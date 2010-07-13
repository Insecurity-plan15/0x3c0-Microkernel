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
    return 0;
}

SystemCallDefinition(execve)
{
    //Receives a collection of memory images
    //All address space outside of heaps is wiped, and memory image loaded
    //Current threads deleted, new thread added
    //  initial values of environment, arguments, EIP, etc
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
    //bleh
}
