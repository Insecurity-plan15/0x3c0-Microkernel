#include <Multitasking/Process.h>
#include <Common/Strings.h>
#include <Multitasking/Security.h>
#include <Memory.h>
#include <MemoryAllocation/AllocateMemory.h>
#include <Common/CPlusPlus.h>
#include <Multitasking/SystemCalls.h>
#include <IPC/MessageCodes.h>

//pd must be a blank page directory, holding only the heap and kernel space
Process::Process(MemoryManagement::x86::PageDirectory pd, Receipt messageMethod, unsigned int f)
{
	pageDir = pd;
	threads = new LinkedList<Thread *>();
	allocator = new MemoryManagement::Heap(UserHeapStart, UserHeapEnd, false, pd);
	priority = DefaultProcessPriority;
	//Set up the security privileges
	securityPrivileges = new Bitmap(SecurityPrivilegeCount);
	sharedPages = new List<SharedPage *>();
	state = ProcessState::Running;
	//The ELF object is uninitialised
	//When a message is received, a popup thread will be created, starting in a copy of this method. It will have its own stack
	onReceipt = messageMethod;

	flags = f;

	//Apply the basic security privileges
	for(unsigned int i = 0; i < sizeof(SecurityPrivilege::DefaultPrivileges) / sizeof(unsigned int); i++)
		securityPrivileges->SetBit(SecurityPrivilege::DefaultPrivileges[i]);

	currentThread = new LinkedListNode<Thread *>(0);

	if(onReceipt != 0)
		flags |= 0x1;
}

Process::~Process()
{
	delete threads;
	delete allocator;
	delete securityPrivileges;
	delete sharedPages;
	if(elfObject != 0)
		delete elfObject;
}

MemoryManagement::x86::PageDirectory Process::GetPageDirectory()
{
	return pageDir;
}

char *Process::GetName()
{
	return name;
}

void Process::SetName(char *n)
{
	Strings::Copy((char *)name, (char *)n, sizeof(name));
}

unsigned int Process::GetProcessId()
{
	return (unsigned int)this;
}

LinkedList<Thread *> *Process::GetThreads()
{
	return threads;
}

MemoryManagement::Heap *Process::GetAllocator()
{
	return allocator;
}

unsigned char Process::GetPriority()
{
	return priority;
}

void Process::SetPriority(unsigned char p)
{
	priority = p;
}

Bitmap *Process::GetSecurityPrivileges()
{
	return securityPrivileges;
}

bool Process::SetSecurityPrivilege(unsigned int privilege)
{
	if((this->state & ProcessState::Kernel) != ProcessState::Kernel)
	{
		for(unsigned int i = 0; i < sizeof(SecurityPrivilege::RestrictedPrivileges) / sizeof(unsigned int); i++)
		{
			if(SecurityPrivilege::RestrictedPrivileges[i] == privilege)
				return false;
		}
	}
	if(securityPrivileges->TestBit(privilege))
		securityPrivileges->ClearBit(privilege);
	else
		securityPrivileges->SetBit(privilege);
	return true;
}

List<SharedPage *> *Process::GetSharedPages()
{
	return sharedPages;
}

unsigned int Process::GetState()
{
	return state;
}

//There must be a safeguard placed here. A process must not be allowed to become a kernel process
void Process::SetState(unsigned int st)
{
	state = st;
}

ELF::ELFObject *Process::GetELFObject()
{
	return elfObject;
}

void Process::SetELFObject(ELF::ELFObject *elf)
{
	elfObject = elf;
}

Receipt Process::GetReceiptMethod()
{
	return onReceipt;
}

void Process::SetReceiptMethod(Receipt onRec)
{
	onReceipt = onRec;
	if(onReceipt != 0)
		flags |= 0x1;
	else
		flags &= ~0x1;
}

bool Process::Kill()
{
	return false;
}

void Process::SendMessage(Message *message)
{
	if(message->GetDestination()->onReceipt == 0)
		return;
	//Just a basic variable; it makes things clearer for me
	Process *sendTo = message->GetDestination();
	//Allocate a block of memory into the destination process' address space which is big enough for the data
	void *data = sendTo->GetAllocator()->Allocate(message->GetLength());
	//Clone the message structure, but point it to the new data
	SystemCalls::UserLevelStructures::Message *m = new (sendTo) SystemCalls::UserLevelStructures::Message();
	//Create the new thread
	Thread *th = new Thread((ThreadStart)sendTo->onReceipt, sendTo, message->GetCode() == MessageCodes::Drivers::InterruptReceived);
	//Find the page directory to copy from. If the source is 0, use the kernel's
	MemoryManagement::x86::PageDirectory pd = message->GetSource() == 0 ? MemoryManagement::Virtual::GetKernelDirectory() :
		message->GetSource()->pageDir;

	//Copy the data over. I can't use a normal CopyMemory here because the virtual addresses will vary.
	//I use my special method which performs a few tricks with paging to speed things up and make the transfer possible
	MemoryManagement::Virtual::CopyToAddressSpace((unsigned int)message->GetData(), message->GetLength(), (unsigned int)data,
		pd);
	m->Data = (unsigned int)data;
	m->Length = message->GetLength();
	m->Code = message->GetCode();
	m->Source = (unsigned int)message->GetSource();
	//Start the new thread
	th->Start(m);
}

void Process::BecomeDriver(Drivers::DriverInfoBlock *d)
{
	if(d != 0 && driver == 0)
		driver = d;
}

Drivers::DriverInfoBlock *Process::GetDriver()
{
	return driver;
}

bool Process::IsDriver()
{
	return driver != 0;
}

unsigned int Process::GetFlags()
{
	return flags;
}
