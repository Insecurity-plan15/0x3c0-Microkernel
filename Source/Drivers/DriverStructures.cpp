#include <Drivers/DriverStructures.h>
#include <Multitasking/Scheduler.h>
#include <IPC/MessageCodes.h>
#include <Common/CPlusPlus.h>

using namespace Drivers;

DriverManager *DriverManager::object = 0;

DriverManager::DriverManager()
{
}

DriverManager::~DriverManager()
{
}

DriverManager *DriverManager::GetDriverManager()
{
	if(object == 0)
		object = new DriverManager();
	return object;
}

void DriverManager::InstallDriver(DriverInfoBlock *block)
{
	if((block->Type & DriverType::Timer) == DriverType::Timer)
	{
		/*
		* This has the potential to tell another part of the OS an AP has booted, but this doesn't happen practically, because
		* the scheduler is booted first
		*/
		Scheduler *scheduler = Scheduler::GetScheduler();

		//If the scheduler doesn't have an associated timer, then this one will do
		if(scheduler->TimerInUse(0))
			scheduler->SetTimer(block);
	}
	if(((block->Type & DriverType::VFS) == DriverType::VFS) && vfs == 0)
		vfs = block;
	LinkedList<DriverInfoBlock *>::Add(block);
	if(block->SupportsInterrupts)
		Interrupts::AddInterrupt(new DriverInterruptSource(block));
}

void DriverManager::RemoveDriver(DriverInfoBlock *block)
{
	LinkedList<DriverInfoBlock *>::Remove(block);
}

DriverInfoBlock *DriverManager::GetVFS()
{
	return vfs;
}

void DriverInterruptSource::receive(StackStates::Interrupt *stack)
{
	Scheduler *sch = Scheduler::GetScheduler();

	if(driver != 0)
	{
		//I allocate the message in the driver's parent's address space
		Message *m = new Message(stack, sizeof(StackStates::Interrupt), 0, driver->Parent, MessageCodes::Drivers::InterruptReceived, 0);

		if(sch != 0 && sch->TimerInUse(driver))
			sch->Yield(stack);
		if(driver->CanReceiveIRQ)
		{
			//Make sure that the driver's parent doesn't get a flood of messages
			driver->CanReceiveIRQ = false;
			//Send a message to the driver's parent to indicate an interrupt has occurred
			driver->Parent->SendMessage(m);
		}
		delete m;
	}
}

DriverInterruptSource::DriverInterruptSource(DriverInfoBlock *block)
	: InterruptSink::InterruptSink(block->IRQ)
{
	driver = block;
}

DriverInterruptSource::~DriverInterruptSource()
{
}
