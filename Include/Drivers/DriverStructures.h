#ifndef DriverInfoBlock_H
#define DriverInfoBlock_H
namespace Drivers { struct DriverInfoBlock; }
#include <Common/Interrupts.h>
#include <Drivers/DriverType.h>
#include <LinkedList.h>
#include <Multitasking/Process.h>

namespace Drivers
{
	//When a process is registered as a driver, it gains the ability to recieve two extra messages:
	//1. IRQ. The parameter is a pointer to the stack
	//2. Load. The parameter is a process ID which identifies what loaded it
	struct DriverInfoBlock
	{
		char Name[32];
		bool SupportsInterrupts;
		unsigned short IRQ;
		unsigned int Type;
		Process *Parent;
		bool CanReceiveIRQ;
	};

	class DriverManager : public LinkedList<DriverInfoBlock *>
	{
	private:
		static DriverManager *object;
		DriverInfoBlock *vfs;
		DriverManager();
		~DriverManager();
	public:
		static DriverManager *GetDriverManager();
		void InstallDriver(DriverInfoBlock *block);
		void RemoveDriver(DriverInfoBlock *block);
		DriverInfoBlock *GetVFS();
	};

	class DriverInterruptSource : public InterruptSink
	{
	private:
		DriverInfoBlock *driver;
		void receive(StackStates::Interrupt *stack);
	public:
		DriverInterruptSource(DriverInfoBlock *block);
		~DriverInterruptSource();
	};
}

#endif
