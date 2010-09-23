#ifndef Interrupts_H
#define Interrupts_H

#include <LinkedList.h>
#include <List.h>
#include <Typedefs.h>

namespace StackStates
{
	struct Interrupt
	{
		cpuRegister DS;
		cpuRegister R15;
		cpuRegister R14;
		cpuRegister R13;
		cpuRegister R12;
		cpuRegister R11;
		cpuRegister R10;
		cpuRegister R9;
		cpuRegister R8;
		cpuRegister RBP;
		cpuRegister RSI;
		cpuRegister RDI;
		cpuRegister RDX;
		cpuRegister RCX;
		cpuRegister RBX;
		cpuRegister RAX;
		cpuRegister InterruptNumber;
		cpuRegister ErrorCode;
		cpuRegister RIP;
		cpuRegister CS;
		cpuRegister RFLAGS;
		cpuRegister RSP;
		cpuRegister SS;					//Pushed by the processor automatically.
	} __attribute__((packed));

	//This isn't laid out properly, but contains everything that a thread's stack will
	struct x86Stack
	{
		cpuRegister CS, DS, SS;
		cpuRegister RAX, RBX, RCX, RDX;
		cpuRegister RBP, RSP;
		cpuRegister RDI, RSI;
		cpuRegister RFLAGS;
		cpuRegister RIP;
		cpuRegister R8, R9, R10, R11, R12, R13, R14, R15;

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
	static LinkedList<InterruptSink *> *GetInterrupts(uint16 vector);
	static void InvokeInterruptChain(uint16 vector, StackStates::Interrupt *stack);
};
#endif
