#ifndef Interrupts_H
#define Interrupts_H

#include <LinkedList.h>
#include <List.h>
#include <Typedefs.h>

namespace StackStates
{
	struct Interrupt
	{
		cpuRegister DS;											//Data segment selector
		cpuRegister RDI, RSI, RBP, RSP, RBX, RDX, RCX, RAX;		//Pushed by pusha.
		cpuRegister InterruptNumber, ErrorCode;					//Interrupt number and error code (if applicable)
		cpuRegister RIP;
		cpuRegister CS, EFLAGS;
		cpuRegister UserESP;
		cpuRegister SS;					//Pushed by the processor automatically.
	} __attribute__((packed));

	//This isn't laid out properly, but contains everything that a thread's stack will
	struct x86Stack
	{
		cpuRegister CS, DS, SS;
		cpuRegister RAX, RBX, RCX, RDX;
		cpuRegister RBP, RSP;
		cpuRegister RDI, RSI;
		cpuRegister EFLAGS;
		cpuRegister UserESP, RIP;

		cpuRegister EntryPoint;
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
