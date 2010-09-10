#include <SystemCalls/POSIXInterface.h>
#include <Multitasking/Scheduler.h>
#include <Multitasking/Process.h>

using namespace SystemCalls::POSIX;

Internal::Internal()
{
}

Internal::~Internal()
{
}

SystemCallDefinition(fork)
{
    Scheduler *sch = Scheduler::GetScheduler();
    Process *current = sch->GetCurrentProcess();
    Thread *currentThread = sch->GetCurrentThread();

    MemoryManagement::x86::PageDirectory forkedPD = MemoryManagement::Virtual::ClonePageDirectory(current->GetPageDirectory(), true);
    Process *forked = new Process(forkedPD, current->GetReceiptMethod(), current->GetState());
    Thread *newThread = new Thread(currentThread->GetEIP(), forked, false);

    /*
    * Note that this includes ESP and EBP. Ordinarily I'd be concerned about bad stack data, but it got copied over when
    * I cloned the page directory
    */
    newThread->SetState( currentThread->GetState() );
    //As per POSIX specifications, fork() returns 0 in the child process
    newThread->GetState()->RAX = 0;
    //Start the thread, with no arguments - they'd be ignored anyway
    newThread->Start(0, 0);
    return (virtAddress)forked;
}

SystemCallDefinition(execve)
{
    //Receives a collection of memory images
    //All address space outside of kernel heaps is wiped, and memory image loaded
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
    unsigned int rip = stack->RIP;
	unsigned int rax = 0;

	if(stack->RDX >= (sizeof(calls) / sizeof(SystemCall)))
	{
		//Somebody's trying to mess up the kernel. Stop them
		stack->RAX = 0xFFFFFFFF;
		return;
	}
	SystemCall sc = calls[stack->RDX];

	/*
	* This takes into account the fact that the execve system call can change the values in the stack. If I change the new stack state,
	* it'll be inconsistent, and in this case the return address will go wobbly.
	*/
	rax = (*sc)(stack->RAX, stack->RBX, stack->RCX, stack);
	if(stack->RIP == rip)
		stack->RAX = rax;
}
