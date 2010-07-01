#ifndef Thread_H
#define Thread_H

#include <Multitasking/Process.h>
#include <Common/Interrupts.h>

typedef void (*ThreadStart)(void *args);

namespace ThreadStatus
{
    const unsigned int Sleeping = 0x1 << 0;

    const unsigned int CustomState = 0x1 << 1;

    const unsigned int IsInterruptMessage = 0x1 << 1;
};

class Thread
{
friend class Scheduler;
private:
	bool sleeping;
	bool customState;
	//If the thread was created as a result of an interrupt message, another interrupt message can only be
	//received when the thread exits
	bool interruptMessage;
	Process *parent;
	StackStates::x86Stack *state;	//This is allocated on the stack
	//It doesn't matter that this method is static - it makes a system call, so the current process' current thread
	//is already selected when interrupts are disabled
	static void returnMethod();
	static void ring3Start(StackStates::x86Stack *stack, void *arg);
public:
	Thread(ThreadStart start, Process *p, bool irqMessage = false);	//start is the initial EIP
	~Thread();
	void Start(void *args);	//args is a non-type-safe parameter which is given as an initial parameter. It can be 0
	void SetState(StackStates::x86Stack *st);
	Process *GetParent();

	bool IsSleeping();
	void Sleep();
	void Wake();

	unsigned int GetEIP();
	unsigned int GetStatus();
	unsigned int GetEntryPoint();
};

#endif