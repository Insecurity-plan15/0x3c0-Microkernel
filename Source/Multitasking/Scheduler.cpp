#include <Multitasking/Scheduler.h>
#include <LinkedList.h>
#include <Boot/DescriptorTables.h>
#include <MemoryAllocation/Physical.h>

List<Scheduler *> *Scheduler::schedulers = 0;
Mutex *Scheduler::schedulerListMutex = 0;

void Scheduler::setKernelStack(unsigned int esp)
{
	DescriptorTables::InstallTSS(esp, getCPUIdentifier());
}

unsigned int Scheduler::getCPUIdentifier()
{
	unsigned int tss = 0;
	//Store the TSS register (STR) and assume that there is one TSS per CPU.
	asm volatile ("str %0" : "=r"(tss));
	//The TSS selector is ORed with the privilege level (3). Remove that OR
	tss &= (~3);
	//If there's no TSS, then the only possibility is that the code's running on the first CPU
	if(tss == 0)
		return 0;
	//Each selector is 0x8 bytes long.
	//The first TSS is the fifth selector
	return (tss / 0x8) - 5;
}

void Scheduler::SetupStack()
{
	void *page = 0;
	unsigned int id = getCPUIdentifier();
	unsigned int stackLocation = 0xF0400000 + (PageSize * id);
	unsigned int mappingFlags = 0;

    //Make certain that the stack isn't already mapped
    MemoryManagement::Virtual::RetrieveMapping((void *)stackLocation, &mappingFlags);

    //If it's present, I've already called SetupStack for this logical core, and don't need to do anything
    if((mappingFlags & MemoryManagement::x86::PageDirectoryFlags::Present) != 0)
        return;
    page = MemoryManagement::Physical::PageAllocator::AllocatePage(false);
	MemoryManagement::Virtual::MapMemory((unsigned int)page, stackLocation, MemoryManagement::x86::PageDirectoryFlags::ReadWrite);
	//Adding PageSize will push the stack into the next page. That isn't mapped.
	//I subtract four because the stack needs to be aligned on a DWORD boundary
	setKernelStack(stackLocation - 4);
}

Scheduler::Scheduler(Process *first)
{
	if(schedulerListMutex == 0)
		schedulerListMutex = new Mutex(true);
	if(first != 0)
	{
		totalPriority = first->GetPriority();
		if((first->GetState() & ProcessState::IOBound) == ProcessState::IOBound)
			totalPriority /= 2;
		currentProcess = first;
		currentThread = first->threads->First->Value;
	}
	else
	{
		totalPriority = 0;
		currentProcess = 0;
		currentThread = 0;
	}
	timer = 0;
	processes = new LinkedList<Process *>(first);
	processListMutex = new Mutex(true);
	sleepingProcesses = new LinkedList<Process *>(0);
	randomGen = new RandomGenerator();
}

Scheduler::~Scheduler()
{
}

void Scheduler::Yield(StackStates::Interrupt *stack)
{
	/*
	* Generate a random number, n, between 0 and totalPriority.
	* Treat it as an index into the address space of total process priorities. Example:
	* _____________________
	* ¦A  ¦     B     ¦ C ¦
	* -------^
	* Here it can clearly be seen that the next process to be run should be B. So keep an accumulator
	* running; when the previous accumulator value is less than or equal to n, and the current accumulator
	* value is greater than or equal to n, I know that the process I need is in place and that I should context switch.
	*
	* Note: The current time complexity is O(N). It'd be nice if I could make this more efficient without
	* scaling down to a round-robin system
	*/
	unsigned int rnd = randomGen->Next(0, totalPriority);
	unsigned int accumulator = 0;

	for(LinkedListNode<Process *> *fst = processes->First; fst != 0 && fst->Value != 0; fst = fst->Next)
	{
		unsigned int effectivePriority = fst->Value->GetPriority();
		unsigned int prevAccumulator = accumulator;

		if((fst->Value->GetState() & ProcessState::IOBound) == ProcessState::IOBound)
			effectivePriority /= 2;
		accumulator += effectivePriority;
		if(rnd >= prevAccumulator && rnd < accumulator)
		{
			YieldTo(fst->Value, stack);
			break;
		}
	}
}

void Scheduler::YieldTo(Process *p, StackStates::Interrupt *stack)
{
	if(p == 0)
		return;
	/*
	* If a process has no threads, then it can't be scheduled. This could make things a little unfair if the process has
	* a very high priority, but since a process is loaded with a single thread which loads the actual executable, I'm not
	* overly worried about this possibility
	*/
	if(p->GetThreads()->GetCount() == 0)
		return;
	//Optimisation alert: if the current process is the same as the one being switched to, there's no need to flush the TLB
	if(p != currentProcess)
	{
		currentProcess = p;
		//Switch address spaces and prepare to hook up the next thread
		asm volatile ("mov %0, %%cr3" : : "r"(currentProcess->GetPageDirectory()));
	}
	//Just iterate through the linked list of threads after carefully selecting the process
	//If I'm at the end of the linked list, then start at the beginning again
	if(currentProcess->currentThread->Next == 0)
		currentProcess->currentThread = currentProcess->threads->First;
	else
		currentProcess->currentThread = currentProcess->currentThread->Next;

	//Keep cycling through the process list until there's a non-sleeping thread.
	//Theoretically, a process filled with sleeping threads could cause a page fault at 0x0, but this cannot happen because
	//there is a separate list for sleeping processes
	for(; currentProcess->currentThread != 0 && currentProcess->currentThread->Value != 0 &&
			currentProcess->currentThread->Value->IsSleeping() ;
			currentProcess->currentThread = currentProcess->currentThread->Next)
		;
	if(currentProcess->currentThread == 0)
		asm volatile ("cli; hlt" : : "a"(0xDEAD));
	if(stack != 0)
	{
		//Save the previous thread state
		currentThread->state->CS = stack->CS;
		currentThread->state->DS = stack->DS;

		currentThread->state->EAX = stack->EAX;
		currentThread->state->EBX = stack->EBX;
		currentThread->state->ECX = stack->ECX;
		currentThread->state->EDX = stack->EDX;

		currentThread->state->ESP = stack->ESP;
		currentThread->state->EBP = stack->EBP;
		currentThread->state->UserESP = stack->UserESP;

		currentThread->state->EDI = stack->EDI;
		currentThread->state->ESI = stack->ESI;

		currentThread->state->EFLAGS = stack->EFLAGS;

		currentThread->state->EIP = stack->EIP;
	}
	else if(currentThread != 0)
	{
		//If there's no stack, and a thread exists to store, then get the values directly from there
		asm volatile ("mov %%eax, %0" : "=r"(currentThread->state->EAX));
		asm volatile ("mov %%ebx, %0" : "=r"(currentThread->state->EBX));
		asm volatile ("mov %%ecx, %0" : "=r"(currentThread->state->ECX));
		asm volatile ("mov %%edx, %0" : "=r"(currentThread->state->EDX));

		asm volatile ("mov %%edi, %0" : "=r"(currentThread->state->EDI));
		asm volatile ("mov %%esi, %0" : "=r"(currentThread->state->ESI));

		asm volatile ("mov %%esp, %0" : "=r"(currentThread->state->ESP));
		asm volatile ("mov %%ebp, %0" : "=r"(currentThread->state->EBP));
	}
	//Switch to the next thread
	currentThread = currentProcess->currentThread->Value;
	//Perform the actual context switch. A stack of zero signifies that the changes should be done on the current stack
	if(stack != 0)
	{
		stack->CS = currentThread->state->CS;
		stack->DS = currentThread->state->DS;
		//I don't alter SS because every process' stack segment is the same

		stack->EAX = currentThread->state->EAX;
		stack->EBX = currentThread->state->EBX;
		stack->ECX = currentThread->state->ECX;
		stack->EDX = currentThread->state->EDX;

		stack->ESP = currentThread->state->ESP;
		stack->EBP = currentThread->state->EBP;
		stack->UserESP = currentThread->state->UserESP;

		stack->EDI = currentThread->state->EDI;
		stack->ESI = currentThread->state->ESI;

		stack->EFLAGS = currentThread->state->EFLAGS;

		stack->EIP = currentThread->state->EIP;
	}
	else
	{
		//There's no need to switch the segment registers because they should remain constant across switches.
		//However, when multitasking is just getting started, I will need to switch to a secondary, ring 3 task
		asm volatile ("mov %0, %%eax" : : "r"(currentThread->state->EAX));
		asm volatile ("mov %0, %%ebx" : : "r"(currentThread->state->EBX));
		asm volatile ("mov %0, %%ecx" : : "r"(currentThread->state->ECX));
		asm volatile ("mov %0, %%edx" : : "r"(currentThread->state->EDX));

		asm volatile ("mov %0, %%edi" : : "r"(currentThread->state->EDI));
		asm volatile ("mov %0, %%esi" : : "r"(currentThread->state->ESI));

		asm volatile ("mov %0, %%esp" : : "r"(currentThread->state->ESP));
		asm volatile ("mov %0, %%ebp" : : "r"(currentThread->state->EBP));
		//There's no need to manipulate EFLAGS, EIP et al because when the stack registers get changed, EIP is found there
	}
	//All these changes get pushed back onto the stack when the interrupt handler completes, changing to another thread
}

void Scheduler::SetTimer(Drivers::DriverInfoBlock *tmr)
{
	timer = tmr;
}

bool Scheduler::TimerInUse(Drivers::DriverInfoBlock *tmr)
{
	return (tmr == timer);
}

Process *Scheduler::GetCurrentProcess()
{
	return currentProcess;
}

Thread *Scheduler::GetCurrentThread()
{
	return currentThread;
}

void Scheduler::AddProcess(Process *p)
{
	if(p != 0)
	{
		processListMutex->Lock();
		processes->Add(p);
		totalPriority += ((p->GetState() & ProcessState::IOBound) == ProcessState::IOBound) ? (p->priority / 2) : p->priority;
		processListMutex->Unlock();
	}
}

void Scheduler::RemoveProcess(Process *p)
{
	//Remove the process from the priority calculations
	unsigned int priority = (p->GetState() & ProcessState::IOBound) == ProcessState::IOBound ? p->priority / 2 : p->priority;

	processListMutex->Lock();
	totalPriority -= priority;
	processes->Remove(p);
	//If the current process has been removed, then perform a simplistic method of returning - just set the first process
	if(currentProcess == p)
	{
		currentProcess = processes->First->Value;
		if(currentProcess->currentThread->Next == 0)
			currentProcess->currentThread = currentProcess->threads->First;
		else
			currentProcess->currentThread = currentProcess->currentThread->Next;
		currentThread = currentProcess->currentThread->Value;
	}
	processListMutex->Unlock();
}

void Scheduler::Sleep(Process *p)
{
	RemoveProcess(p);
	sleepingProcesses->Add(p);
}

void Scheduler::Wake(Process *p)
{
	//Unfortunately, this is a necessary hack. I return 0 if I can't find the element I want. Zero is also a valid
	//index, so I check the First field.
	//If the process is not present in the list of sleeping processes, it's already awake and doesn't need waking again
	if(sleepingProcesses->First->Value != p && sleepingProcesses->Find(p) == 0)
		return;
	sleepingProcesses->Remove(p);
	AddProcess(p);
}

Scheduler *Scheduler::GetScheduler()
{
	unsigned int id = getCPUIdentifier();

	//If the first core is booting, then the list won't exist. Don't tell the rest of the OS that an AP has booted
	if(schedulers == 0)
	{
		Scheduler *addSched = new Scheduler(0);

		schedulerListMutex->Lock();
		schedulers = new List<Scheduler *>();
		schedulers->Add(addSched);
		schedulerListMutex->Unlock();
		return addSched;
	}
	//If the total number of items is greater than id, the instance can't exist, and needs to be created
	if(schedulers->GetItem(id) == 0)
	{
		unsigned int max = id > schedulers->GetCount() ? id : schedulers->GetCount();
		unsigned int min = id < schedulers->GetCount() ? id : schedulers->GetCount();
		//Processor IDs are always consecutive

		//If something gets to this point, then an AP has booted
		if(max - min == 1)
		{
			Scheduler *newScheduler = new Scheduler(0);

			schedulerListMutex->Lock();
			schedulers->Add(newScheduler);
			schedulerListMutex->Unlock();
			return newScheduler;
		}
		else
		{
			Scheduler *newScheduler = new Scheduler(0);

			schedulerListMutex->Lock();
			//If the ID isn't consecutive, then fill in the gaps
			for(unsigned int i = 0; i < max - min; i++)
				schedulers->Add(0);
			schedulers->Add(newScheduler);
			schedulerListMutex->Unlock();
			return newScheduler;
		}
	}
	else
		return schedulers->GetItem(id);
}

LinkedList<Process *> *Scheduler::GetAllProcesses()
{
	LinkedList<Process *> *allProcesses = new LinkedList<Process *>();

	//Prevent the list of schedulers from changing, as it could on a multi-core system
	schedulerListMutex->Lock();
	//Now that everything's sane, iterate through each scheduler..
	for(unsigned int i = 0; i < schedulers->GetCount(); i++)
	{
		Scheduler *sch = schedulers->GetItem(i);

		//..and prevent them from messing with the lists we're about to enumerate
		sch->processListMutex->Lock();
		for(LinkedListNode<Process *> *nd = sch->processes->First; nd != 0 && nd->Value != 0; nd = nd->Next)
			allProcesses->Add(nd->Value);
		sch->processListMutex->Unlock();
	}
	schedulerListMutex->Unlock();
	return allProcesses;
}
