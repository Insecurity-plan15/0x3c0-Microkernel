#include <Multitasking/Thread.h>
#include <MemoryAllocation/Physical.h>
#include <Multitasking/Paging.h>
#include <Common/CPlusPlus.h>

void Thread::returnMethod()
{
	//Perform a 'kill thread' system call, passing the current thread ID as a parameter
	asm volatile ("int $0x30" : : "d"(0x1), "a"(0));
	//If the system call failed, then cause a GPF by executing privileged instructions. The kernel will kill the thread
	asm volatile ("xchg %%bx, %%bx" : : "a"(0x400));
	asm volatile ("cli; hlt" : : "a"(0xABD));
}

/*
* This method is unfortunately necessary. The x86 architecture will only allow the entry of ring 3 from a few scenarios.
* One of these scenarios is the invocation of an IRET. After I enter ring 3, I set up a stack frame, then jump to the
* thread's method
*/

//Hmm. Odd. I want to push a set of variables, not an array of them. How do I do this (variadic parameters?)
__attribute__((aligned(0x1000))) void Thread::ring3Start(StackStates::x86Stack *state, ...)
{
	//This snippet has been lifted by JamesM's kernel development tutorials (with some adaptations)
	//All copyright goes to him
	/*
	* I can't add comments alongside the ASM block, so I'm putting them here
	* 1) I push the EAX register, so that what I'm about to put in the lower 16 bits doesn't overwrite anything
	* 2) I load the segment registers with the correct ring 3 values
	* 3) I restore EAX
	* Then, I begin to set up a stack frame for IRET
	* 1) I get the correct value of ESP
	* 2) IRET needs me to push SS (0x20 | 0x3), ESP, EFLAGS, CS (0x18 | 0x3) and EIP in that order. So I do
	* 3) I execute the IRET
	* 4) The stack seems to get very messed up, so I try to correct it beforehand. This could very easily be wrong,
	*	or change between compiler revisions. However, it works here
	*/
	unsigned int CS = 0;

	asm volatile ("mov %%cs, %0" : "=g"(CS));

	asm volatile("			\
		cli;				\
		push %%edx;			\
		mov $0x23, %%dx;	\
		mov %%dx, %%ds;		\
		mov %%dx, %%es;		\
		mov %%dx, %%fs;		\
		mov %%dx, %%gs;		\
		pop %%edx;			\
							\
		push %0;			\
		push %1;			\
		push %2;			\
		push %3;			\
		push $1f;			\
		iret;				\
	1:						\
		add $0x10, %%esp;	\
		push %4;			\
		push %5;			\
		push %6;			\
		push %7;			\
		push %8;			\
		push %9;			\
		sub $0x24, %%esp" : : "g"(state->DS), "g"(state->ESP), "g"(state->EFLAGS), "g"(state->CS), "g"(&Thread::returnMethod),
								"g"(state->EntryPoint), "g"(state->EBP), "g"(state->EDI), "g"(state->ESI), "g"(state->EBX) : "%edx", "%ecx");
	/*
	* The code which begins to run at label 1: is running in ring 3.
	* This stack hackery makes me a little bit nervous, but it seems valid. I've tested with a variety of method signatures,
	* with a variety of parameters. Everything _appears_ to work. The correct sequence for pushing to set up a stack frame is:
	* 1) Add 0x10 to ESP, to counteract the ending stub GCC adds
	* 2) Push return address
	* 3) Push the necessary EIP
	* 4) Push 4 integers. These are EBP, EDI, ESI and EDX
	* 5) Subtract 0x24 from ESP, to put the stack back as GCC expects
	*/
}

__attribute__((aligned(0x1000))) Thread::Thread(ThreadStart start, Process *p, bool irqMessage)
{
	parent = p;
	state = new StackStates::x86Stack();
	state->EntryPoint = (unsigned int)start;
	state->CS = 0x18 | 3;	//0x18 points to the user code segment, ORed with the privilege level
	state->DS = 0x20 | 3;	//User data segment, OR privilege level
	state->ESP = state->EBP = state->EIP = 0;
	asm volatile ("pushf; pop %0" : "=r"(state->EFLAGS));

	//Set the Interrupt Enabled bit in EFLAGS, just as STI does
	state->EFLAGS |= 0x200;
	//Now set the IO privilege level to zero, to prevent any ring3 threads from accessing the IO ports
	state->EFLAGS &= ~0x3000;

	customState = false;
	sleeping = true;
	interruptMessage = irqMessage;
}

Thread::~Thread()
{
	//Free the stack that was created to prevent any angry memory leaks
	if(state->ESP != 0)
		//The magic number 5 comes from the fact that there are five decrements when the thread gets started
		delete[] (unsigned int *)(state->ESP - (0x1000 - 5 * 0x4));
}

void Thread::Start(void **args, unsigned int argCount)
{
	/*
	* This is somewhat complex stack manipulation. Here's a diagram of what the stack should look like
	* http://unixwiz.net/techtips/win32-callconv-asm.html
	* I add 0x1000 because ESP and EBP point to the end of the stack
	*/
	unsigned int *stackState = (unsigned int *)(new(parent) unsigned int[0x1000]) + 0x1000;
	unsigned int virtualMapping[2] = { 0, 0 };
	//This criteria might go strange once I introduce multiple process
	bool userModeActive = parent->threads->GetCount() != 0;

	//A custom state means that we can rely on the process to have set up the basic stuff, and can start the thread immediately
	if(!customState)
	{
		//The first things to go on the stack are the arguments
		//The below code is quite messy, but I don't know any better way to do it
		for(unsigned int i = argCount; i > 0; i--)
		    *--stackState = (unsigned int)(args[i - 1]);
		if(! userModeActive)
			*--stackState = (unsigned int)state;

		//After that is the return address, which will basically just use a system call to kill the process
		*--stackState = (unsigned int)&Thread::returnMethod;

		//Below that is the current EIP
		*--stackState = userModeActive ? state->EntryPoint : (unsigned int)&Thread::ring3Start;
		//The current EBP (0) goes at the bottom of the stack
		*--stackState = 0;

		state->EIP = userModeActive ? state->EntryPoint : (unsigned int)&Thread::ring3Start;

		state->ESP = state->EBP = userModeActive ? (unsigned int)(stackState + 2) : (unsigned int)stackState;
		state->UserESP = state->ESP;
	}
	/*
	* Since the thread starts executing kernel code, it needs to be able to access it
	* I'm not going to make the entire kernel available to user-space, but I will expose the two pages
	* containing the routine which drops to ring 3. There's very little it can do to anything there anyway
	*/
	virtualMapping[0] = MemoryManagement::Virtual::RetrieveMapping((void *)&Thread::returnMethod, &virtualMapping[1]);
	MemoryManagement::Virtual::MapMemory(virtualMapping[0], (unsigned int)&Thread::returnMethod, virtualMapping[1] | MemoryManagement::x86::PageDirectoryFlags::UserAccessible);

	virtualMapping[0] = MemoryManagement::Virtual::RetrieveMapping((void *)((unsigned int)&Thread::returnMethod + PageSize), &virtualMapping[1]);
	MemoryManagement::Virtual::MapMemory(virtualMapping[0], (unsigned int)&Thread::returnMethod + PageSize, virtualMapping[1] | MemoryManagement::x86::PageDirectoryFlags::UserAccessible);
	//Now that the stack is set up, the parent process just needs to know about its new thread
	parent->threads->Add(this);
	//If we're the parent's first thread, then the parent needs to be told that it's got a thread
	if(parent->currentThread->Value == 0 && parent->threads->GetCount() == 1)
		parent->currentThread->Value = this;
	//Allow the scheduler to begin to schedule this thread
	sleeping = false;
}

void Thread::SetState(StackStates::x86Stack *st)
{
	unsigned int esp = state->ESP;
	unsigned int ebp = state->EBP;

	state = st;
	if(st->ESP != 0)
		customState = true;
	else
	{
		st->ESP = esp;
		st->EBP = ebp;
	}
}

Process *Thread::GetParent()
{
	return parent;
}

bool Thread::IsSleeping()
{
	return sleeping;
}

void Thread::Sleep()
{
	sleeping = true;
}

void Thread::Wake()
{
	sleeping = false;
}

unsigned int Thread::GetEIP()
{
	return state->EIP;
}

unsigned int Thread::GetStatus()
{
	unsigned int status = 0;

	if(sleeping)
		status |= ThreadStatus::Sleeping;
	if(customState)
		status |= ThreadStatus::CustomState;
    if(interruptMessage)
        status |= ThreadStatus::IsInterruptMessage;
	return status;
}

unsigned int Thread::GetEntryPoint()
{
	return state->EntryPoint;
}
