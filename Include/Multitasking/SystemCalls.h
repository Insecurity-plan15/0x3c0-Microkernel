#ifndef SystemCalls_H
#define SystemCalls_H

#include <Common/Interrupts.h>

#define TotalSystemCalls	18

typedef unsigned int (*SystemCall)(unsigned int eax, unsigned int ebx, unsigned int ecx, StackStates::Interrupt *stack);

namespace SystemCalls
{
	namespace UserLevelStructures
	{
		struct Message
		{
			unsigned int Data;
			unsigned int Length;
			unsigned int Code;
			unsigned int Source;
		} __attribute__((packed));

		struct MemoryBlock
		{
			unsigned int Start;
			unsigned int End;
			unsigned int CopyTo;
		} __attribute__((packed));

		struct DriverInfoBlock
		{
			char Name[32];
			bool SupportsInterrupts;
			unsigned short IRQ;
			unsigned int Type;
		} __attribute__((packed));

		struct SharedPage
		{
		} __attribute__((packed));

		//This namespace contains the structure definitions used by the RequestProcessData system call
		namespace ProcessData
		{
			struct Thread
			{
				unsigned int TID;
				unsigned int EIP;
				unsigned int Status;
			} __attribute__((packed));

			struct MemoryUsageStructure
			{
				long long MemoryUsed;
				unsigned int SharedPageCount;
			} __attribute__((packed));

			struct Process
			{
				char Name[32];
				unsigned int PID;
				DriverInfoBlock *Driver;
				unsigned int Flags;
				MemoryUsageStructure *MemoryUsage;
				unsigned int ThreadCount;
				Thread *Threads;
			} __attribute__((packed));

			struct ReturnValue
			{
				//This is the process that the VFS runs as. All FS-related messages must be sent here
				unsigned int VFSPointer;
				unsigned int CurrentPID;
				unsigned int ProcessCount;
				Process *Processes;
			} __attribute__((packed));
		}
	}

	class Internal
	{
		#define SystemCallMethod(_name)	static unsigned int _name(unsigned int eax, unsigned int ebx, unsigned int ecx, StackStates::Interrupt *stack)
	private:
		Internal();
		~Internal();
		static unsigned int isAddressValid(unsigned int address, bool checkUserMode);
	public:
		SystemCallMethod(CreateThread);
		SystemCallMethod(KillThread);
		SystemCallMethod(CreateProcess);
		SystemCallMethod(KillProcess);
		SystemCallMethod(SendMessage);
		SystemCallMethod(CreateSharedPage);
		SystemCallMethod(DestroySharedPage);
		SystemCallMethod(RequestCPUBootStub);
		SystemCallMethod(RequestIdentityMapping);
		SystemCallMethod(InstallDriver);
		SystemCallMethod(Yield);
		SystemCallMethod(PerformPageManagement);
		SystemCallMethod(SetReceiptMethod);
		SystemCallMethod(ToggleSecurityPrivilege);
		SystemCallMethod(SecurityPrivilegeStatus);
		SystemCallMethod(SleepThread);
		SystemCallMethod(WakeThread);
		SystemCallMethod(RequestProcessData);
	};

	class SystemCallInterrupt : public InterruptSink
	{
	private:
		SystemCall calls[TotalSystemCalls];
		void receive(StackStates::Interrupt *stack);
	public:
		SystemCallInterrupt();
		~SystemCallInterrupt();
	};
}

#endif
