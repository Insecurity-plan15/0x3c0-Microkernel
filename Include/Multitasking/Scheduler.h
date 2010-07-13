#ifndef Scheduler_H
#define Scheduler_H

#include <Multitasking/Process.h>
#include <Multitasking/Thread.h>
#include <Drivers/DriverStructures.h>
#include <Common/Interrupts.h>
#include <Common/RandomGenerator.h>
#include <LinkedList.h>
#include <List.h>
//#include <Multitasking/SystemCalls.h>
#include <IPC/Mutex.h>

class Scheduler
{
friend class Process;
friend class SystemCalls::Native::Internal;
private:
	void setKernelStack(unsigned int esp);
	static unsigned int getCPUIdentifier();
	//For computers with support for SMP, I store a list of schedulers
	static List<Scheduler *> *schedulers;
	//Also for SMP support, this is used when enumerating the list of schedulers
	static Mutex *schedulerListMutex;
	//To save time when switching threads, I store the total priority instead of calculating it
	unsigned int totalPriority;
	//This is the timer that the process scheduler Yield()s on
	Drivers::DriverInfoBlock *timer;
	//I use this to quickly calculate a random number
	RandomGenerator *randomGen;
	//The list of processes in the scheduler
	LinkedList<Process *> *processes;
	LinkedList<Process *> *sleepingProcesses;
	Process *currentProcess;
	Thread *currentThread;

	//Used when enumerating the process list, so that the state is consistent
	Mutex *processListMutex;
public:
	Scheduler(Process *first);
	~Scheduler();
	/*
	* Yield will only manipulate the stack variable. Changing tasks in an IRQ handler is just a matter of
	* passing the IRQ handler's StackState parameter. To do an immediate task switch, just pass 0;
	* the 'live' registers will be updated
	*/
	void Yield(StackStates::Interrupt *stack);
	void YieldTo(Process *p, StackStates::Interrupt *stack);
	//Connected to the system calls somehow
	void SetTimer(Drivers::DriverInfoBlock *tmr);
	//If a timer is in use and its IRQ has fired, then the scheduler gets called first
	bool TimerInUse(Drivers::DriverInfoBlock *tmr);
	Process *GetCurrentProcess();
	Thread *GetCurrentThread();

	void AddProcess(Process *p);
	void RemoveProcess(Process *p);

	void Sleep(Process *p);
	void Wake(Process *p);

	//When a processor boots, it calls this method, which retrieves the processor ID and retrieves a Scheduler instance
	static Scheduler *GetScheduler();
	//This gets called whenever the kernel wants to know every process running in the system
	static LinkedList<Process *> *GetAllProcesses();

	void SetupStack();
};

#endif
