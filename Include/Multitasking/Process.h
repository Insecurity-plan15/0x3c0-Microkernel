#ifndef Process_H
#define Process_H

//These are dummy declarations. They get overwritten when the proper object is declared
class Process;
class Thread;
namespace ELF {	class ELFObject; }

#include <Multitasking/Paging.h>
#include <Multitasking/Thread.h>
#include <MemoryAllocation/AllocateMemory.h>
#include <LinkedList.h>
#include <List.h>
#include <IPC/Message.h>
#include <Multitasking/MemoryMap.h>
#include <Bitmap.h>
#include <Drivers/DriverStructures.h>
#include <ELF/ELFHeaders.h>
#include <IPC/SharedPage.h>
#include <Multitasking/SystemCalls.h>

typedef void (*Receipt)(struct Message *);

#define DefaultProcessPriority	1

class SharedObject;

namespace ProcessState
{
	//IOBound: Waiting for an IO event to occur. The priority is effectively halved
	const unsigned int IOBound = 0x1;
	//Running: Normal
	const unsigned int Running = 0x2;
	//Zombie: No threads, finished running, needs cleaning up
	const unsigned int Zombie = 0x4;
	//Kernel: Can respond to a process' requests for restricted privileges
	const unsigned int Kernel = 0x8;
}

class Process
{
friend class Scheduler;
friend class Thread;
friend class SystemCalls::Internal;
private:
	MemoryManagement::x86::PageDirectory pageDir;
	char name[32];
	LinkedList<Thread *> *threads;
	MemoryManagement::Heap *allocator;
	unsigned char priority;
	Bitmap *securityPrivileges;
	List<SharedPage *> *sharedPages;
	unsigned int state;	//This can take one of the values in the ProcessState namespace
	MemoryMap *memoryMap;
	ELF::ELFObject *elfObject;
	Receipt onReceipt;
	Drivers::DriverInfoBlock *driver;
	LinkedListNode<Thread *> *currentThread;
	//If a process cannot receive messages, bit 0 is 0
	unsigned int flags;
public:
	Process(MemoryManagement::x86::PageDirectory pd, Receipt messageMethod, unsigned int f = 0);
	~Process();
	MemoryManagement::x86::PageDirectory GetPageDirectory();	//Used to manipulate the page directory and implement shared pages

	char *GetName();		//A non-unique name which identifies a process to a user
	void SetName(char *n);

	unsigned int GetProcessId();	//This is a unique value - the virtual address of the process

	LinkedList<Thread *> *GetThreads();	//A list of all threads (including v86 ones) in a process

	MemoryManagement::Heap *GetAllocator();	//All pages freed when process exits

	unsigned char GetPriority();		//Integer from 0 to 50. The higher the process, the more airtime it gets
	void SetPriority(unsigned char p);

	Bitmap *GetSecurityPrivileges();
	bool SetSecurityPrivilege(unsigned int privilege);

	List<SharedPage *> *GetSharedPages();

	unsigned int GetState();	//If bit 0 is set, the process is IO bound and effectively needs to have a halved priority
	void SetState(unsigned int st);

	MemoryMap *GetMemoryMap();	//Contains a layout of all memory according to the process

	ELF::ELFObject *GetELFObject();	//Used for dynamic linking
	void SetELFObject(ELF::ELFObject *elf);

	Receipt GetReceiptMethod();
	void SetReceiptMethod(Receipt onRec);

	bool Kill();

	void SendMessage(Message *message);

	void BecomeDriver(Drivers::DriverInfoBlock *d);
	Drivers::DriverInfoBlock *GetDriver();
	bool IsDriver();

	unsigned int GetFlags();
};
#endif
