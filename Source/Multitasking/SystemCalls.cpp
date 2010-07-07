#include <Multitasking/Security.h>
#include <MemoryAllocation/Physical.h>
#include <Multitasking/SystemCalls.h>
#include <Multitasking/Process.h>
#include <Multitasking/Thread.h>
#include <Multitasking/Paging.h>
#include <Multitasking/Scheduler.h>
#include <Memory.h>
#include <Drivers/DriverStructures.h>
#include <Common/CPlusPlus.h>

#define SystemCallDefinition(_name)	unsigned int Internal::_name(unsigned int eax, unsigned int ebx, unsigned int ecx, StackStates::Interrupt *stack)

using namespace SystemCalls;

unsigned int Internal::isAddressValid(unsigned int address, bool checkUserMode)
{
	unsigned int flags = 0;

	MemoryManagement::Virtual::RetrieveMapping((void *)address, &flags);
	if((flags & MemoryManagement::x86::PageDirectoryFlags::Present) == 0)
		return 1;
	if(checkUserMode && ((flags & MemoryManagement::x86::PageDirectoryFlags::UserAccessible) == 0))
		return 2;
	return 0;
}

Internal::Internal()
{
}

Internal::~Internal()
{
}

SystemCallDefinition(CreateThread)
{
	unsigned int valid = isAddressValid(eax, true);
	Scheduler *sched = Scheduler::GetScheduler();
	Process *p = sched->GetCurrentProcess();
	Thread *thread;

	if(valid != 0)
		return valid;
	//If the process doesn't have permission to create a thread, don't go any further
	if(! p->GetSecurityPrivileges()->TestBit(SecurityPrivilege::CreateThread))
		return 3;
	thread = new Thread((ThreadStart)eax, sched->GetCurrentProcess());
	//Get the current process from the current CPUs scheduler, and create a thread
	thread->Start((void **)ebx);
	sched->Wake(p);
	return 0;
}

SystemCallDefinition(KillThread)
{
	Scheduler *sched = Scheduler::GetScheduler();
	Thread *thread = (Thread *)eax;
	Process *p = 0;

	//Passing a 0 in EAX will kill the current thread
	if(eax != 0)
	{
		unsigned int valid = isAddressValid(eax, true);

		if(valid != 0)
			return valid;
	}
	else
 		thread = sched->GetCurrentThread();
	p = thread->GetParent();

	//If the thread is for an interrupt message, the process is the current process, the process is a driver,
	//and the driver cannot currently receive IRQs, then give it that ability
	if( ((thread->GetStatus() & ThreadStatus::IsInterruptMessage) == ThreadStatus::IsInterruptMessage) && (eax == 0) &&
            p->IsDriver() && (!p->GetDriver()->CanReceiveIRQ) )
		p->GetDriver()->CanReceiveIRQ = true;

	p->GetThreads()->Remove(thread);
	if(eax == 0)
		sched->currentThread = 0;
	if(p->GetThreads()->GetCount() == 0)
		sched->RemoveProcess(p);
	return 0;
}

SystemCallDefinition(CreateProcess)
{
	Scheduler *sched = Scheduler::GetScheduler();
	unsigned int valid = 0;
	UserLevelStructures::MemoryBlock *blocks = (UserLevelStructures::MemoryBlock *)eax;
	unsigned int elementCount = ebx;
	MemoryManagement::x86::PageDirectory newPageDir = 0;
	Process *p = 0;
	Thread *th = 0;

	if(! sched->GetCurrentProcess()->GetSecurityPrivileges()->TestBit(SecurityPrivilege::CreateProcess))
		return 3;
	//Check that the elements in the list are valid
	for(unsigned int i = 0; i < elementCount * sizeof(UserLevelStructures::MemoryBlock); i += PageSize)
	{
		valid = isAddressValid((unsigned int)blocks + i, true);
		if(valid != 0)
			return valid;
	}
	//Now that the list is probably valid, make sure that the elements within the list aren't pointing to stupid addresses
	for(unsigned int i = 0; i < ebx; i++)
	{
		UserLevelStructures::MemoryBlock *currentBlock = &blocks[i];

		if(currentBlock->Start > currentBlock->End)
			return 4;
		if(currentBlock->End > UserHeapStart)
			return 5;
		for(unsigned int j = currentBlock->Start; j < currentBlock->End; j += PageSize)
		{
			//If the address isn't present in the current address space, then just drop out immediately
			valid = isAddressValid(j, true);
			if(valid != 0)
				return valid;
		}
	}
	//Okay, error checking is out of the way - move on to the main event of loading the process
	//Clone the kernel page directory and create a new process based on it
	newPageDir = MemoryManagement::Virtual::ClonePageDirectory();
	p = new Process(newPageDir, 0);
	//Iterate through every memory block and copy the contents over
	for(unsigned int i = 0; i < ebx; i++)
	{
		UserLevelStructures::MemoryBlock *currentBlock = &blocks[i];

		//Although the destination address doesn't have to be present, it still needs to be user mode or Bad Things will happen
		MemoryManagement::Virtual::CopyToAddressSpace(currentBlock->Start, currentBlock->Start - currentBlock->End, currentBlock->CopyTo,
			false, true);
	}
	//If ECX isn't valid, the process will page fault and be immediately killed
	th = new Thread((ThreadStart)ecx, p);

	th->Start((void **)0);

	sched->AddProcess(p);
	return 0;
}

SystemCallDefinition(KillProcess)
{
	Scheduler *sched = Scheduler::GetScheduler();
	Process *p = eax == 0 ? sched->GetCurrentProcess() : (Process *)eax;
	unsigned int valid = isAddressValid(eax, false);

	if(valid != 0)
		return valid;
	sched->RemoveProcess(p);
	for(LinkedListNode<Thread *> *nd = p->GetThreads()->First; nd != 0 && nd->Value != 0; nd = nd->Next)
		delete nd->Value;
	delete p;
	sched->Yield(stack);
	return 0;
}

SystemCallDefinition(SendMessage)
{
	Scheduler *sched = Scheduler::GetScheduler();
	Process *p = sched->GetCurrentProcess();
	unsigned int valid = isAddressValid(eax, true);
	UserLevelStructures::Message *internalMsg = (UserLevelStructures::Message *)eax;
	Message *msg = 0;

	if(valid != 0)
		return valid;
	if(! p->GetSecurityPrivileges()->TestBit(SecurityPrivilege::MessagePassing))
		return 3;
	for(unsigned int i = internalMsg->Data; i < internalMsg->Data + internalMsg->Length; i += PageSize)
	{
		valid = isAddressValid(i, true);
		if(valid != 0)
			return valid;
	}
	valid = isAddressValid(ebx, true);
	if(valid != 0)
		return valid;
	msg = new Message((void *)internalMsg->Data, internalMsg->Length, p, (Process *)ebx, internalMsg->Code);
	p->SendMessage(msg);
	delete msg;
	return 0;
}

SystemCallDefinition(CreateSharedPage)
{
	//For a shared page to be created, it must not exist in both the current process' and destination process' address space
	//The source and all destination processes must call this system call
//	Scheduler *sched = Scheduler::GetScheduler();
	unsigned int valid = isAddressValid(eax, true);
//	UserLevelStructures::SharedPage *pg = (UserLevelStructures::SharedPage *)eax;
//	Process *p = sched->GetCurrentProcess();

	if(valid != 0)
		return valid;
	return 0;
}

SystemCallDefinition(DestroySharedPage)
{
	return 0;
}

SystemCallDefinition(RequestCPUBootStub)
{
	return 0;
}

SystemCallDefinition(RequestIdentityMapping)
{
	unsigned int flags = 0;

	for(unsigned int i = eax; i < eax + ebx; eax += PageSize)
	{
		MemoryManagement::Virtual::RetrieveMapping((void *)i, &flags);
		if((flags & MemoryManagement::x86::PageDirectoryFlags::Present) != 0)
			//Mapping already present, not going to allow it to be removed.
			return 1;
	}
	//Do the loop again, but this time perform the mapping
	for(unsigned int i = eax; i < eax + ebx; eax += PageSize)
		MemoryManagement::Virtual::MapMemory(eax, MemoryManagement::x86::PageDirectoryFlags::UserAccessible);
	return 0;
}

SystemCallDefinition(InstallDriver)
{
	Scheduler *sched = Scheduler::GetScheduler();
	unsigned int valid = isAddressValid(eax, true);
	Drivers::DriverInfoBlock *driver;
	UserLevelStructures::DriverInfoBlock *original = (UserLevelStructures::DriverInfoBlock *)eax;
	Drivers::DriverManager *driverManager = Drivers::DriverManager::GetDriverManager();

	if(valid != 0)
		return valid;
	//I allocate here to avoid memory leaks if a bad address is passed
	driver = new Drivers::DriverInfoBlock();
	//Copy over the data into kernel structures
	/*
	* This relies on the fact that the user-level structure is the same as the kernel-level structure, but lacks a Parent field.
	* The layout of the structure must be identical, or small glitches will occur
	*/
	Memory::Copy(driver, original, sizeof(UserLevelStructures::DriverInfoBlock));
	driver->Parent = sched->GetCurrentProcess();
	driverManager->InstallDriver(driver);
	//Set the IOPL to 3, so that the driver can use port IO
	//TODO: This isn't the best way to do this. I need to consider alternative options, such as the IO permission bitmap
	stack->EFLAGS |= 0x3000;
	return 0;
}

SystemCallDefinition(Yield)
{
	Scheduler *sched = Scheduler::GetScheduler();

	if(! sched->GetCurrentProcess()->GetSecurityPrivileges()->TestBit(SecurityPrivilege::Yield))
		return 3;
	sched->Yield(stack);
	return 0;
}

SystemCallDefinition(PerformPageManagement)
{
	unsigned int error = isAddressValid(eax, false);
	void *p = 0;

	//If the page is not present, then isAddressValid will return 1
	if(error != 1)
		return 1;
	//There's no way that an application or driver should be allowed to manipulate kernel addresses
	if(ebx >= 0xD000000)
		return 2;
	//If EBX is 1, then allocate a physical page and return its address in EBX
	switch(ebx)
	{
        case 1:
        {
            p = MemoryManagement::Physical::PageAllocator::AllocatePage(true);
            if(p == 0)
                return 3;
            stack->EBX = (unsigned int)p;
            return 0;
        }
        case 2:
        {
            unsigned int data[2];

            data[0] = MemoryManagement::Virtual::RetrieveMapping((void *)eax, &data[1]);
            if((data[1] & MemoryManagement::x86::PageDirectoryFlags::Present) == 0)
                return 3;
            MemoryManagement::Physical::PageAllocator::FreePage((void *)data[0]);
            MemoryManagement::Virtual::EraseMapping((void *)eax);
            return 0;
        }
	}
	//Allocate a page and map it in
	p = MemoryManagement::Physical::PageAllocator::AllocatePage(false);
	MemoryManagement::Virtual::MapMemory((unsigned int)p, eax, MemoryManagement::x86::PageDirectoryFlags::Present |
		MemoryManagement::x86::PageDirectoryFlags::ReadWrite | MemoryManagement::x86::PageDirectoryFlags::UserAccessible);
	return 0;
}

SystemCallDefinition(SetReceiptMethod)
{
	Scheduler *sched = Scheduler::GetScheduler();
	unsigned int isValid = isAddressValid(eax, true);
	Process *p = sched->GetCurrentProcess();

	if(isValid != 0)
		return isValid;
	p->SetReceiptMethod((Receipt)eax);
	return 0;
}

SystemCallDefinition(ToggleSecurityPrivilege)
{
	Scheduler *sched = Scheduler::GetScheduler();
	Process *p = sched->GetCurrentProcess();

	//SecurityPrivilegeCount is the total number of elements in the array. I want the maximum index
	if(eax > SecurityPrivilegeCount - 1)
		return 4;
	//SetSecurityPrivilege will only fail for one reason - a non-kernel process attempting to usurp privileges
	if(! p->SetSecurityPrivilege(eax))
		return 5;
	return 0;
}

SystemCallDefinition(SecurityPrivilegeStatus)
{
	Scheduler *sched = Scheduler::GetScheduler();
	Process *p = sched->GetCurrentProcess();
	Bitmap *security = p->GetSecurityPrivileges();

	if(eax > SecurityPrivilegeCount - 1)
		return 4;
	stack->EBX = security->TestBit(eax);
	return 0;
}

SystemCallDefinition(SleepThread)
{
	Scheduler *sched = Scheduler::GetScheduler();
	Process *p = sched->GetCurrentProcess();
	Thread *th = sched->GetCurrentThread();
	bool allThreadsSleeping = true;

	//Put the current thread to sleep
	th->Sleep();
	//If every thread in the current process is sleeping, the process needs to be put to sleep
	for(LinkedListNode<Thread *> *node = p->GetThreads()->First; node != 0 && node->Value != 0; node = node->Next)
	{
		if(! node->Value->IsSleeping())
		{
			allThreadsSleeping = false;
			break;
		}
	}
	//If every thread is inactive, then the process needs to be put to sleep
	if(allThreadsSleeping)
	{
		sched->Sleep(p);
		sched->Yield(stack);
	}
	return 0;
}

SystemCallDefinition(WakeThread)
{
	Scheduler *sched = Scheduler::GetScheduler();
	Thread *th = (Thread *)eax;
	unsigned int valid = isAddressValid(eax, true);

	if(valid != 0)
		return valid;
	th->Wake();
	sched->Wake(th->GetParent());
	return 0;
}

SystemCallDefinition(RequestProcessData)
{
#define BitSet(integer, bit) (((integer) & (1 << (bit))) != 0)
	unsigned int flags = eax;
	LinkedList<Process *> *processes = 0;
	Scheduler *sch = Scheduler::GetScheduler();
	Process *p = sch->GetCurrentProcess();
	UserLevelStructures::ProcessData::ReturnValue *returnData = 0;
	//To synchronise across multiple cores: lock the lists of processes. Can't decide whether to use a mutex or a semaphore
	//All wakes, sleeps, process addition, process killing will need it to be unlocked. When the list data has been extracted, unlock
	//Also lock the list of schedulers. Do that first, and the process locking last

	//EBX input is flags
	//Bit 0: provide VFS process
	//Bit 1: Provide process listing
	//Bit 2: Provide thread listing
	//Bit 3: Provide driver listing
	//Bit 4: Provide memory usage structure
	if(!BitSet(flags, 1) && BitSet(flags, 2))
		return 4;
	if(!BitSet(flags, 1) && BitSet(flags, 3))
		return 4;
	if(!BitSet(flags, 1) && BitSet(flags, 4))
		return 4;

	returnData = new (p) UserLevelStructures::ProcessData::ReturnValue();

	if(BitSet(flags, 1))
	{
		//I'll get a linker error here. Let it serve as a reminder ;)
		processes = Scheduler::GetAllProcesses();
		returnData->ProcessCount = processes->GetCount();
	}
	else
	{
		returnData->ProcessCount = 0;
	}

	//The VFS process is managed by the driver manager. There can only be one, and the VFS process exists forever
	if(BitSet(flags, 0))
	{
		Drivers::DriverInfoBlock *vfs = Drivers::DriverManager::GetDriverManager()->GetVFS();

		//Obvious reasons here. If vfs is 0, accessing vfs->Parent will cause a page fault
		returnData->VFSPointer = vfs != 0 ? (unsigned int)vfs->Parent : 0;
	}
	else
		returnData->VFSPointer = 0;

	if(BitSet(flags, 1))
	{
		unsigned int i = 0;

		returnData->Processes = new (p) UserLevelStructures::ProcessData::Process[returnData->ProcessCount];

		for(LinkedListNode<Process *> *node = processes->First; node != 0 && node->Value != 0; node = node->Next, i++)
		{
			Memory::Copy((void *)&returnData->Processes[i].Name, node->Value->GetName(), sizeof(returnData->Processes[i].Name));
			returnData->Processes[i].PID = (unsigned int)node->Value;
			if(BitSet(flags, 3))
			{
				if(node->Value->GetDriver() == 0)
					returnData->Processes[i].Driver = 0;
				else
				{
					UserLevelStructures::DriverInfoBlock *drv = new (p) UserLevelStructures::DriverInfoBlock();

					Memory::Copy((void *)drv, node->Value->GetDriver(), sizeof(UserLevelStructures::DriverInfoBlock));
					returnData->Processes[i].Driver = drv;
				}
			}
			else
				returnData->Processes[i].Driver = 0;
			returnData->Processes[i].Flags = node->Value->GetFlags();
			if(BitSet(flags, 4))
			{
				UserLevelStructures::ProcessData::MemoryUsageStructure *memUsage = new (p) UserLevelStructures::ProcessData::MemoryUsageStructure();

				memUsage->MemoryUsed = node->Value->GetAllocator()->GetAllocatedMemory();
				memUsage->SharedPageCount = node->Value->GetSharedPages()->GetCount();
				returnData->Processes[i].MemoryUsage = memUsage;
			}
			else
				returnData->Processes[i].MemoryUsage = 0;

			if(BitSet(flags, 2))
			{
				unsigned int j = 0;

				returnData->Processes[i].Threads = new (p) UserLevelStructures::ProcessData::Thread[node->Value->GetThreads()->GetCount()];
				for(LinkedListNode<Thread *> *threadNode = node->Value->GetThreads()->First; node != 0 && node->Value != 0; node = node->Next, j++)
				{
					returnData->Processes[i].Threads[j].TID = (unsigned int)threadNode->Value;
					returnData->Processes[i].Threads[j].EIP = threadNode->Value->GetEIP();
					returnData->Processes[i].Threads[j].Status = threadNode->Value->GetStatus();
				}
			}
			else
			{
				returnData->Processes[i].ThreadCount = 0;
				returnData->Processes[i].Threads = 0;
			}
		}
	}
	returnData->CurrentPID = BitSet(flags, 6) ? (unsigned int)Scheduler::GetScheduler()->GetCurrentProcess() : 0;
	stack->EBP = (unsigned int)returnData;
	return 0;
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
	* This takes into account the fact that the Yield system call can change the values in the stack. If I change the new stack state,
	* it'll be inconsistent, and in this case the return address will go wobbly.
	*/
	eax = (*sc)(stack->EAX, stack->EBX, stack->ECX, stack);
	if(stack->EIP == eip)
		stack->EAX = eax;
}

SystemCallInterrupt::SystemCallInterrupt()
	: InterruptSink::InterruptSink(0x30)
{
	calls[0] = Internal::CreateThread;
	calls[1] = Internal::KillThread;
	calls[2] = Internal::CreateProcess;
	calls[3] = Internal::KillProcess;
	calls[4] = Internal::SendMessage;
	calls[5] = Internal::CreateSharedPage;
	calls[6] = Internal::DestroySharedPage;
	calls[7] = Internal::RequestCPUBootStub;
	calls[8] = Internal::RequestIdentityMapping;
	calls[9] = Internal::InstallDriver;
	calls[10] = Internal::Yield;
	calls[11] = Internal::PerformPageManagement;
	calls[12] = Internal::SetReceiptMethod;
	calls[13] = Internal::ToggleSecurityPrivilege;
	calls[14] = Internal::SecurityPrivilegeStatus;
	calls[15] = Internal::SleepThread;
	calls[16] = Internal::WakeThread;
	calls[17] = Internal::RequestProcessData;
}

InterruptSink::~InterruptSink()
{
}

SystemCallInterrupt::~SystemCallInterrupt()
{
}
