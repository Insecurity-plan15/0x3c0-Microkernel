#ifndef Interrupts_H
#define Interrupts_H

#include <LinkedList.h>
#include <List.h>

namespace StackStates
{
	struct Interrupt
	{
		unsigned int DS;											//Data segment selector
		unsigned int EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX;		//Pushed by pusha.
		unsigned int InterruptNumber, ErrorCode;					//Interrupt number and error code (if applicable)
		unsigned int EIP, CS, EFLAGS, UserESP, SS;					//Pushed by the processor automatically.
	} __attribute__((packed));

	//This isn't laid out properly, but contains everything that a thread's stack will
	struct x86Stack
	{
		unsigned int CS, DS, SS;
		unsigned int EAX, EBX, ECX, EDX;
		unsigned int EBP, ESP;
		unsigned int EDI, ESI;
		unsigned int EFLAGS, UserESP;
		unsigned int EIP;

		unsigned int EntryPoint;
	};
}

class InterruptSink
{
friend class Interrupts;
private:
	unsigned short v;
	virtual void receive(StackStates::Interrupt *stack) = 0;
public:
	InterruptSink(unsigned short vector)
	{
		v = vector;
	}
	virtual ~InterruptSink() = 0;

	unsigned short GetVector()
	{
		return v;
	}
};

class Interrupts
{
private:
	static List<LinkedList<InterruptSink *> *> *interrupts;
	Interrupts();
	~Interrupts();
public:
	static void Install();
	static void AddInterrupt(InterruptSink *sink);
	static void RemoveInterrupt(InterruptSink *sink);
	static LinkedList<InterruptSink *> *GetInterrupts(unsigned short vector);
	static void InvokeInterruptChain(unsigned short vector, StackStates::Interrupt *stack);
};
#endif
